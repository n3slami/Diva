/*
 * Copyright (c) ---
 *
 * All rights reserved. No warranty, explicit or implicit, provided.
 */
#define _GNU_SOURCE

#define BITMASK(nbits)                                    \
  ((nbits) == 64 ? 0xffffffffffffffff : ((1ull << nbits) - 1))

// headers {{{
#include <assert.h> // static_assert
#include "kv.h"
#include "lib.h"
#include "ctypes.h"
#include "wh_int.h"
#include <x86intrin.h>
// }}} headers

// def {{{
#define WH_HMAPINIT_SIZE ((1u << 12)) // 10: 16KB/64KB  12: 64KB/256KB  14: 256KB/1MB
#define WH_SLABMETA_SIZE ((1lu << 21)) // 2MB

#ifndef HEAPCHECKING
#define WH_SLABLEAF_SIZE ((1lu << 21)) // 2MB is ok
#else
#define WH_SLABLEAF_SIZE ((1lu << 21)) // 2MB for valgrind
#endif

#define WH_KPN ((32u)) // keys per node; power of 2
#define WH_HDIV (((1u << 16)) / WH_KPN)
#define WH_MID ((WH_KPN >> 1)) // ideal cut point for split, the closer the better
#define WH_BKT_NR ((8))
#define WH_KPN2 ((WH_KPN + WH_KPN))

#define WH_KPN_MRG (((WH_KPN + WH_MID) >> 1 )) // 3/4

// FO is fixed at 256. Don't change it
#define WH_FO  ((256u)) // index fan-out
// number of bits in a bitmap
#define WH_BMNR ((WH_FO >> 6)) // number of u64
// }}} def

// struct {{{
struct wormmeta {
  struct entry13 k13; // kref+klen
  struct entry13 l13; // lmost+bitmin+bitmax
  struct entry13 r13; // rmost+hash32_lo
  struct entry13 p13; // lpath+hash32_hi
  u64 bitmap[0]; // 4 if bitmin != bitmax
};
static_assert(sizeof(struct wormmeta) == 32, "sizeof(wormmeta) != 32");

struct wormkv64 { u64 key; void * ptr; }; // u64 keys (whu64)

struct store_sim_hack {
  uint32_t a;
  uint64_t b;
};

struct wormleaf_int {
  // first line
  rwlock leaflock;
  spinlock sortlock; // to protect the seemingly "read-only" iter_seek
  au64 lv; // version (dont use the first u64)
  struct wormleaf_int * prev; // prev leaf
  struct wormleaf_int * next; // next leaf
  struct kv * anchor;

  u32 nr_sorted;
  u32 nr_keys;
  u64 reserved[2];

  struct int_store_pair {
      u8 key_size;
      u64 key;
      u8 store[sizeof(struct store_sim_hack)];
  } kvs[WH_KPN]; 
};

struct wormslot { u16 t[WH_BKT_NR]; };
static_assert(sizeof(struct wormslot) == 16, "sizeof(wormslot) != 16");

struct wormmbkt { struct wormmeta * e[WH_BKT_NR]; };
static_assert(sizeof(struct wormmbkt) == 64, "sizeof(wormmbkt) != 64");

struct wormhmap {
  au64 hv;
  struct wormslot * wmap;
  struct wormmbkt * pmap;
  u32 mask;
  u32 maxplen;
  u64 msize;

  struct slab * slab1;
  struct slab * slab2;
  struct kv * pbuf;
};
static_assert(sizeof(struct wormhmap) == 64, "sizeof(wormhmap) != 64");

struct wormhole_int {
  // 1 line
  union {
    au64 hmap_ptr; // safe
    struct wormhmap * hmap; // unsafe
  };
  u64 padding0[6];
  struct wormleaf_int * leaf0; // usually not used
  // 1 line
  struct kvmap_mm mm;
  struct qsbr * qsbr;
  struct slab * slab_leaf;
  struct kv * pbuf;
  u32 leaftype;
  u32 padding1;
  // 2 lines
  struct wormhmap hmap2[2];
  // fifth line
  rwlock metalock;
  u32 padding2[15];
};

// }}} struct

// helpers {{{

  inline static int 
compare_int_isp(u64 a, u8 alen, const struct int_store_pair * const b) 
{
    const u64 bkey = _bswap64(b->key);
    if (a != bkey)
        return a < bkey ? -1 : 1;
    if (alen == b->key_size)
        return 0;
    return alen < b->key_size ? -1 : 1;
}

  static inline u32
isp_key_lcp(const struct int_store_pair * const key1, const struct int_store_pair * const key2)
{
  const u32 max = (key1->key_size < key2->key_size) ? key1->key_size : key2->key_size;
  return memlcp((const u8 *) &(key1->key), (const u8 *) &(key2->key), max);
}

  inline static int 
compare_isp_isp(const struct int_store_pair * const a, const struct int_store_pair * const b) 
{
    const u64 akey = _bswap64(a->key);
    const u64 bkey = _bswap64(b->key);
    if (akey != bkey)
        return akey < bkey ? -1 : 1;
    if (a->key_size == b->key_size)
        return 0;
    return a->key_size < b->key_size ? -1 : 1;
}

// meta {{{
  static inline struct kv *
wormmeta_keyref_load(const struct wormmeta * const meta)
{
  return u64_to_ptr(meta->k13.e3);
}

  static inline u16
wormmeta_klen_load(const struct wormmeta * const meta)
{
  return meta->k13.e1;
}

  static inline struct wormleaf_int *
wormmeta_lmost_load(const struct wormmeta * const meta)
{
  return u64_to_ptr(meta->l13.e3 & (~0x3flu));
}

  static inline u32
wormmeta_bitmin_load(const struct wormmeta * const meta)
{
  return (u32)(meta->l13.v64 & 0x1fflu);
}

  static inline u32
wormmeta_bitmax_load(const struct wormmeta * const meta)
{
  return (u32)((meta->l13.v64 >> 9) & 0x1fflu);
}

  static inline u32
wormmeta_hash32_load(const struct wormmeta * const meta)
{
  return ((u32)meta->r13.e1) | (((u32)meta->p13.e1) << 16);
}

  static inline struct wormleaf_int *
wormmeta_rmost_load(const struct wormmeta * const meta)
{
  return u64_to_ptr(meta->r13.e3);
}

  static inline struct wormleaf_int *
wormmeta_lpath_load(const struct wormmeta * const meta)
{
  return u64_to_ptr(meta->p13.e3);
}

// internal
  static inline void
wormmeta_lpath_store(struct wormmeta * const meta, struct wormleaf_int * const leaf)
{
  entry13_update_e3(&meta->p13, ptr_to_u64(leaf));
}

// also updates leaf_klen_eq and
  static inline void
wormmeta_lmost_store(struct wormmeta * const meta, struct wormleaf_int * const leaf)
{
  const u64 minmax = meta->l13.v64 & 0x3fffflu;
  meta->l13.v64 = (((u64)leaf) << 16) | minmax;

  const bool leaf_klen_eq = leaf->anchor->klen == wormmeta_klen_load(meta);
  wormmeta_lpath_store(meta, leaf_klen_eq ? leaf : leaf->prev);
}

  static inline void
wormmeta_bitmin_store(struct wormmeta * const meta, const u32 bitmin)
{
  meta->l13.v64 = (meta->l13.v64 & (~0x1fflu)) | bitmin;
}

  static inline void
wormmeta_bitmax_store(struct wormmeta * const meta, const u32 bitmax)
{
  meta->l13.v64 = (meta->l13.v64 & (~0x3fe00lu)) | (bitmax << 9);
}

  static inline void
wormmeta_rmost_store(struct wormmeta * const meta, struct wormleaf_int * const leaf)
{
  entry13_update_e3(&meta->r13, ptr_to_u64(leaf));
}

// for wormmeta_alloc
  static void
wormmeta_init(struct wormmeta * const meta, struct wormleaf_int * const lrmost,
    struct kv * const keyref, const u32 alen, const u32 bit)
{
  keyref->refcnt++; // shared

  const u32 plen = keyref->klen;
  debug_assert(plen <= UINT16_MAX);
  meta->k13 = entry13((u16)plen, ptr_to_u64(keyref));
  meta->l13.v64 = (ptr_to_u64(lrmost) << 16) | (bit << 9) | bit;

  const u32 hash32 = keyref->hashlo;
  meta->r13 = entry13((u16)hash32, ptr_to_u64(lrmost));

  const bool leaf_klen_eq = alen == plen;
  meta->p13 = entry13((u16)(hash32 >> 16), ptr_to_u64(leaf_klen_eq ? lrmost : lrmost->prev));
}
// }}} meta

// meta-bitmap {{{
  static inline bool
wormmeta_bm_test(const struct wormmeta * const meta, const u32 id)
{
  debug_assert(id < WH_FO);
  const u32 bitmin = wormmeta_bitmin_load(meta);
  const u32 bitmax = wormmeta_bitmax_load(meta);
  if (bitmin == bitmax) { // half node
    return bitmin == id;
  } else { // full node
    return (bool)((meta->bitmap[id >> 6u] >> (id & 0x3fu)) & 1lu);
  }
}

// meta must be a full node
  static void
wormmeta_bm_set(struct wormmeta * const meta, const u32 id)
{
  // need to replace meta
  u64 * const ptr = &(meta->bitmap[id >> 6u]);
  const u64 bit = 1lu << (id & 0x3fu);
  if ((*ptr) & bit)
    return;

  (*ptr) |= bit;

  // min
  if (id < wormmeta_bitmin_load(meta))
    wormmeta_bitmin_store(meta, id);

  // max
  const u32 oldmax = wormmeta_bitmax_load(meta);
  if (oldmax == WH_FO || id > oldmax)
    wormmeta_bitmax_store(meta, id);
}

// find the lowest bit > id0
// return WH_FO if not found
  static inline u32
wormmeta_bm_gt(const struct wormmeta * const meta, const u32 id0)
{
  u32 ix = id0 >> 6;
  u64 bits = meta->bitmap[ix] & ~((1lu << (id0 & 0x3fu)) - 1lu);
  if (bits)
    return (ix << 6) + (u32)__builtin_ctzl(bits);

  while (++ix < WH_BMNR) {
    bits = meta->bitmap[ix];
    if (bits)
      return (ix << 6) + (u32)__builtin_ctzl(bits);
  }

  return WH_FO;
}

// find the highest bit that is lower than the id0
// return WH_FO if not found
  static inline u32
wormmeta_bm_lt(const struct wormmeta * const meta, const u32 id0)
{
  u32 ix = id0 >> 6;
  u64 bits = meta->bitmap[ix] & ((1lu << (id0 & 0x3fu)) - 1lu);
  if (bits)
    return (ix << 6) + 63u - (u32)__builtin_clzl(bits);

  while (ix--) {
    bits = meta->bitmap[ix];
    if (bits)
      return (ix << 6) + 63u - (u32)__builtin_clzl(bits);
  }

  return WH_FO;
}

// meta must be a full node
  static inline void
wormmeta_bm_clear(struct wormmeta * const meta, const u32 id)
{
  debug_assert(wormmeta_bitmin_load(meta) < wormmeta_bitmax_load(meta));
  meta->bitmap[id >> 6u] &= (~(1lu << (id & 0x3fu)));

  // min
  if (id == wormmeta_bitmin_load(meta))
    wormmeta_bitmin_store(meta, wormmeta_bm_gt(meta, id));

  // max
  if (id == wormmeta_bitmax_load(meta))
    wormmeta_bitmax_store(meta, wormmeta_bm_lt(meta, id));
}
// }}} meta-bitmap

// key/prefix {{{
  static inline u16
wormhole_pkey(const u32 hash32)
{
  const u16 pkey0 = ((u16)hash32) ^ ((u16)(hash32 >> 16));
  return pkey0 ? pkey0 : 1;
}

  static inline u32
wormhole_bswap(const u32 hashlo)
{
  return __builtin_bswap32(hashlo);
}

  static inline bool
wormhole_key_meta_match(const struct kv * const key, const struct wormmeta * const meta)
{
  return (key->klen == wormmeta_klen_load(meta))
    && (!memcmp(key->kv, wormmeta_keyref_load(meta)->kv, key->klen));
}

// called by get_kref_slot
  static inline bool
wormhole_kref_meta_match(const struct kref * const kref,
    const struct wormmeta * const meta)
{
  return (kref->len == wormmeta_klen_load(meta))
    && (!memcmp(kref->ptr, wormmeta_keyref_load(meta)->kv, kref->len));
}

// called from meta_down ... get_kref1_slot
// will access rmost, prefetching is effective here
  static inline bool
wormhole_kref1_meta_match(const struct kref * const kref,
    const struct wormmeta * const meta, const u8 cid)
{
  const u8 * const keybuf = wormmeta_keyref_load(meta)->kv;
  const u32 plen = kref->len;
  return ((plen + 1) == wormmeta_klen_load(meta))
    && (!memcmp(kref->ptr, keybuf, plen))
    && (keybuf[plen] == cid);
}

// warning: be careful with buffer overflow
  static inline void
wormhole_prefix(struct kv * const pfx, const u32 klen)
{
  pfx->klen = klen;
  kv_update_hash(pfx);
}

// for split
  static inline void
wormhole_prefix_inc1(struct kv * const pfx)
{
  pfx->hashlo = crc32c_u8(pfx->hashlo, pfx->kv[pfx->klen]);
  pfx->klen++;
}

// meta_lcp only
  static inline void
wormhole_kref_inc(struct kref * const kref, const u32 len0,
    const u32 crc, const u32 inc)
{
  kref->hash32 = crc32c_inc(kref->ptr + len0, inc, crc);
  kref->len = len0 + inc;
}

// meta_lcp only
  static inline void
wormhole_kref_inc_123(struct kref * const kref, const u32 len0,
    const u32 crc, const u32 inc)
{
  kref->hash32 = crc32c_inc_123(kref->ptr + len0, inc, crc);
  kref->len = len0 + inc;
}
// }}} key/prefix

// alloc {{{
  static inline struct kv *
wormhole_alloc_akey(const size_t klen)
{
#ifdef ALLOCFAIL
  if (alloc_fail())
    return NULL;
#endif
  return malloc(sizeof(struct kv) + klen);
}

  static inline void
wormhole_free_akey(struct kv * const akey)
{
  free(akey);
}

  static inline struct kv *
wormhole_alloc_mkey(const size_t klen)
{
#ifdef ALLOCFAIL
  if (alloc_fail())
    return NULL;
#endif
  return malloc(sizeof(struct kv) + klen);
}

  static inline void
wormhole_free_mkey(struct kv * const mkey)
{
  free(mkey);
}

  static struct wormleaf_int *
wormleaf_int_alloc(struct wormhole_int * const map, struct wormleaf_int * const prev,
    struct wormleaf_int * const next, struct kv * const anchor)
{
  struct wormleaf_int * const leaf = slab_alloc_safe(map->slab_leaf);
  if (leaf == NULL)
    return NULL;

  rwlock_init(&(leaf->leaflock));
  spinlock_init(&(leaf->sortlock));

  // keep the old version; new version will be assigned by split functions
  //leaf->lv = 0;

  leaf->prev = prev;
  leaf->next = next;
  leaf->anchor = anchor;

  leaf->nr_keys = 0;
  leaf->nr_sorted = 0;

  // hs requires zero init.
  memset(leaf->kvs, 0, sizeof(leaf->kvs[0]) * WH_KPN);
  return leaf;
}

  static void
wormleaf_int_free(struct slab * const slab, struct wormleaf_int * const leaf)
{
  debug_assert(leaf->leaflock.opaque == 0);
  wormhole_free_akey(leaf->anchor);
  slab_free_safe(slab, leaf);
}

  static struct wormmeta *
wormmeta_alloc(struct wormhmap * const hmap, struct wormleaf_int * const lrmost,
    struct kv * const keyref, const u32 alen, const u32 bit)
{
  debug_assert(alen <= UINT16_MAX);
  debug_assert(lrmost && keyref);

  struct wormmeta * const meta = slab_alloc_unsafe(hmap->slab1);
  if (meta == NULL)
    return NULL;

  wormmeta_init(meta, lrmost, keyref, alen, bit);
  return meta;
}

  static inline bool
wormhole_slab_reserve(struct wormhole_int * const map, const u32 nr)
{
#ifdef ALLOCFAIL
  if (alloc_fail())
    return false;
#endif
  for (u32 i = 0; i < 2; i++) {
    if (!(map->hmap2[i].slab1 && map->hmap2[i].slab2))
      continue;
    if (!slab_reserve_unsafe(map->hmap2[i].slab1, nr))
      return false;
    if (!slab_reserve_unsafe(map->hmap2[i].slab2, nr))
      return false;
  }
  return true;
}

  static void
wormmeta_keyref_release(struct wormmeta * const meta)
{
  struct kv * const keyref = wormmeta_keyref_load(meta);
  debug_assert(keyref->refcnt);
  keyref->refcnt--;
  if (keyref->refcnt == 0)
    wormhole_free_mkey(keyref);
}

  static void
wormmeta_free(struct wormhmap * const hmap, struct wormmeta * const meta)
{
  wormmeta_keyref_release(meta);
  slab_free_unsafe(hmap->slab1, meta);
}
// }}} alloc

// lock {{{
  static void
wormleaf_int_lock_write(struct wormleaf_int * const leaf, struct wormref_int * const ref)
{
    
  if (!rwlock_trylock_write(&(leaf->leaflock))) {
    wormhole_int_park(ref);
    rwlock_lock_write(&(leaf->leaflock));
    wormhole_int_resume(ref);
  }
}

  static void
wormleaf_int_lock_read(struct wormleaf_int * const leaf, struct wormref_int * const ref)
{
  if (!rwlock_trylock_read(&(leaf->leaflock))) {
    wormhole_int_park(ref);
    rwlock_lock_read(&(leaf->leaflock));
    wormhole_int_resume(ref);
  }
}

  static void
wormleaf_int_unlock_write(struct wormleaf_int * const leaf)
{
  rwlock_unlock_write(&(leaf->leaflock));
}

// HAAAAAAAAAAAAAAAAAAAAAAACK
  void
wormleaf_int_unlock_read(struct wormleaf_int * const leaf)
{
  rwlock_unlock_read(&(leaf->leaflock));
}

  static void
wormhmap_lock(struct wormhole_int * const map, struct wormref_int * const ref)
{
  if (!rwlock_trylock_write(&(map->metalock))) {
    wormhole_int_park(ref);
    rwlock_lock_write(&(map->metalock));
    wormhole_int_resume(ref);
  }
}

  static inline void
wormhmap_unlock(struct wormhole_int * const map)
{
  rwlock_unlock_write(&(map->metalock));
}
// }}} lock

// hmap-version {{{
  static inline struct wormhmap *
wormhmap_switch(struct wormhole_int * const map, struct wormhmap * const hmap)
{
  return (hmap == map->hmap2) ? (hmap + 1) : (hmap - 1);
}

  static inline struct wormhmap *
wormhmap_load(struct wormhole_int * const map)
{
  return (struct wormhmap *)atomic_load_explicit(&(map->hmap_ptr), MO_ACQUIRE);
}

  static inline void
wormhmap_store(struct wormhole_int * const map, struct wormhmap * const hmap)
{
  atomic_store_explicit(&(map->hmap_ptr), (u64)hmap, MO_RELEASE);
}

  static inline u64
wormhmap_version_load(const struct wormhmap * const hmap)
{
  // no concurrent access
  return atomic_load_explicit(&(hmap->hv), MO_ACQUIRE);
}

  static inline void
wormhmap_version_store(struct wormhmap * const hmap, const u64 v)
{
  atomic_store_explicit(&(hmap->hv), v, MO_RELEASE);
}

  static inline u64
wormleaf_int_version_load(struct wormleaf_int * const leaf)
{
  return atomic_load_explicit(&(leaf->lv), MO_CONSUME);
}

  static inline void
wormleaf_int_version_store(struct wormleaf_int * const leaf, const u64 v)
{
  atomic_store_explicit(&(leaf->lv), v, MO_RELEASE);
}
// }}} hmap-version

// co {{{
  static inline struct wormmeta *
wormhmap_get_meta(const struct wormhmap * const hmap, const u32 mid, const u32 i)
{
  return hmap->pmap[mid].e[i];
}

  static inline void
wormleaf_int_prefetch(struct wormleaf_int * const leaf, const u32 hashlo)
{
  const u32 i = wormhole_pkey(hashlo) / WH_HDIV;
#if defined(CORR)
  cpu_prefetch0(leaf);
  cpu_prefetch0(&(leaf->hs[i-4]));
  cpu_prefetch0(&(leaf->hs[i+4]));
  corr_yield();
#else
  cpu_prefetch0(&(leaf->kvs[i]));
#endif
}

  static inline bool
wormhole_kref_kv_match(const struct kref * const key, const struct kv * const curr)
{
  return kref_kv_match(key, curr);
}

  static inline void
wormhole_qsbr_update_pause(struct wormref_int * const ref, const u64 v)
{
  qsbr_update(&ref->qref, v);
#if defined(CORR)
  corr_yield();
#endif
}
// }}} co

// }}} helpers

// hmap {{{
// hmap is the MetaTrieHT of Wormhole
  static bool
wormhmap_init(struct wormhmap * const hmap, struct kv * const pbuf)
{
  const u64 wsize = sizeof(hmap->wmap[0]) * WH_HMAPINIT_SIZE;
  const u64 psize = sizeof(hmap->pmap[0]) * WH_HMAPINIT_SIZE;
  u64 msize = wsize + psize;
  u8 * const mem = pages_alloc_best(msize, true, &msize);
  if (mem == NULL)
    return false;

  hmap->pmap = (typeof(hmap->pmap))mem;
  hmap->wmap = (typeof(hmap->wmap))(mem + psize);
  hmap->msize = msize;
  hmap->mask = WH_HMAPINIT_SIZE - 1;
  wormhmap_version_store(hmap, 0);
  hmap->maxplen = 0;
  hmap->pbuf = pbuf;
  return true;
}

  static inline void
wormhmap_deinit(struct wormhmap * const hmap)
{
  if (hmap->pmap) {
    pages_unmap(hmap->pmap, hmap->msize);
    hmap->pmap = NULL;
    hmap->wmap = NULL;
  }
}

  static inline m128
wormhmap_zero(void)
{
#if defined(__x86_64__)
  return _mm_setzero_si128();
#elif defined(__aarch64__)
  return vdupq_n_u8(0);
#endif
}

  static inline m128
wormhmap_m128_pkey(const u16 pkey)
{
#if defined(__x86_64__)
  return _mm_set1_epi16((short)pkey);
#elif defined(__aarch64__)
  return vreinterpretq_u8_u16(vdupq_n_u16(pkey));
#endif
}

  static inline u32
wormhmap_match_mask(const struct wormslot * const s, const m128 skey)
{
#if defined(__x86_64__)
  const m128 sv = _mm_load_si128((const void *)s);
  return (u32)_mm_movemask_epi8(_mm_cmpeq_epi16(skey, sv));
#elif defined(__aarch64__)
  const uint16x8_t sv = vld1q_u16((const u16 *)s); // load 16 bytes at s
  const uint16x8_t cmp = vceqq_u16(vreinterpretq_u16_u8(skey), sv); // cmpeq => 0xffff or 0x0000
  static const uint16x8_t mbits = {0x3, 0xc, 0x30, 0xc0, 0x300, 0xc00, 0x3000, 0xc000};
  return (u32)vaddvq_u16(vandq_u16(cmp, mbits));
#endif
}

  static inline bool
wormhmap_match_any(const struct wormslot * const s, const m128 skey)
{
#if defined(__x86_64__)
  return wormhmap_match_mask(s, skey) != 0;
#elif defined(__aarch64__)
  const uint16x8_t sv = vld1q_u16((const u16 *)s); // load 16 bytes at s
  const uint16x8_t cmp = vceqq_u16(vreinterpretq_u16_u8(skey), sv); // cmpeq => 0xffff or 0x0000
  return vaddvq_u32(vreinterpretq_u32_u16(cmp)) != 0;
#endif
}

// meta_lcp only
  static inline bool
wormhmap_peek(const struct wormhmap * const hmap, const u32 hash32)
{
  const m128 sk = wormhmap_m128_pkey(wormhole_pkey(hash32));
  const u32 midx = hash32 & hmap->mask;
  const u32 midy = wormhole_bswap(hash32) & hmap->mask;
  return wormhmap_match_any(&(hmap->wmap[midx]), sk)
    || wormhmap_match_any(&(hmap->wmap[midy]), sk);
}

  static inline struct wormmeta *
wormhmap_get_slot(const struct wormhmap * const hmap, const u32 mid,
    const m128 skey, const struct kv * const key)
{
  u32 mask = wormhmap_match_mask(&(hmap->wmap[mid]), skey);
  while (mask) {
    const u32 i2 = (u32)__builtin_ctz(mask);
    struct wormmeta * const meta = wormhmap_get_meta(hmap, mid, i2>>1);
    if (likely(wormhole_key_meta_match(key, meta)))
      return meta;
    mask ^= (3u << i2);
  }
  return NULL;
}

  static struct wormmeta *
wormhmap_get(const struct wormhmap * const hmap, const struct kv * const key)
{
  const u32 hash32 = key->hashlo;
  const u32 midx = hash32 & hmap->mask;
  const u32 midy = wormhole_bswap(hash32) & hmap->mask;
  const m128 skey = wormhmap_m128_pkey(wormhole_pkey(hash32));

  struct wormmeta * const r = wormhmap_get_slot(hmap, midx, skey, key);
  if (r)
    return r;
  return wormhmap_get_slot(hmap, midy, skey, key);
}

// for meta_lcp only
  static inline struct wormmeta *
wormhmap_get_kref_slot(const struct wormhmap * const hmap, const u32 mid,
    const m128 skey, const struct kref * const kref)
{
  u32 mask = wormhmap_match_mask(&(hmap->wmap[mid]), skey);
  while (mask) {
    const u32 i2 = (u32)__builtin_ctz(mask);
    struct wormmeta * const meta = wormhmap_get_meta(hmap, mid, i2>>1);
    if (likely(wormhole_kref_meta_match(kref, meta)))
      return meta;

    mask ^= (3u << i2);
  }
  return NULL;
}

// for meta_lcp only
  static inline struct wormmeta *
wormhmap_get_kref(const struct wormhmap * const hmap, const struct kref * const kref)
{
  const u32 hash32 = kref->hash32;
  const u32 midx = hash32 & hmap->mask;
  const u32 midy = wormhole_bswap(hash32) & hmap->mask;
  const m128 skey = wormhmap_m128_pkey(wormhole_pkey(hash32));

  struct wormmeta * const r = wormhmap_get_kref_slot(hmap, midx, skey, kref);
  if (r)
    return r;
  return wormhmap_get_kref_slot(hmap, midy, skey, kref);
}

// for meta_down only
  static inline struct wormmeta *
wormhmap_get_kref1_slot(const struct wormhmap * const hmap, const u32 mid,
    const m128 skey, const struct kref * const kref, const u8 cid)
{
  u32 mask = wormhmap_match_mask(&(hmap->wmap[mid]), skey);
  while (mask) {
    const u32 i2 = (u32)__builtin_ctz(mask);
    struct wormmeta * const meta = wormhmap_get_meta(hmap, mid, i2>>1);
    if (likely(wormhole_kref1_meta_match(kref, meta, cid)))
      return meta;

    mask ^= (3u << i2);
  }
  return NULL;
}

// for meta_down only
  static inline struct wormmeta *
wormhmap_get_kref1(const struct wormhmap * const hmap,
    const struct kref * const kref, const u8 cid)
{
  const u32 hash32 = crc32c_u8(kref->hash32, cid);
  const u32 midx = hash32 & hmap->mask;
  const u32 midy = wormhole_bswap(hash32) & hmap->mask;
  const m128 skey = wormhmap_m128_pkey(wormhole_pkey(hash32));

  struct wormmeta * const r = wormhmap_get_kref1_slot(hmap, midx, skey, kref, cid);
  if (r)
    return r;
  return wormhmap_get_kref1_slot(hmap, midy, skey, kref, cid);
}

  static inline u32
wormhmap_slot_count(const struct wormslot * const slot)
{
  const u32 mask = wormhmap_match_mask(slot, wormhmap_zero());
  return mask ? ((u32)__builtin_ctz(mask) >> 1) : 8;
}

  static inline void
wormhmap_squeeze(const struct wormhmap * const hmap)
{
  struct wormslot * const wmap = hmap->wmap;
  struct wormmbkt * const pmap = hmap->pmap;
  const u32 mask = hmap->mask;
  const u64 nrs64 = ((u64)(hmap->mask)) + 1; // must use u64; u32 can overflow
  for (u64 si64 = 0; si64 < nrs64; si64++) { // # of buckets
    const u32 si = (u32)si64;
    u32 ci = wormhmap_slot_count(&(wmap[si]));
    for (u32 ei = ci - 1; ei < WH_BKT_NR; ei--) {
      struct wormmeta * const meta = pmap[si].e[ei];
      const u32 sj = wormmeta_hash32_load(meta) & mask; // first hash
      if (sj == si)
        continue;

      // move
      const u32 ej = wormhmap_slot_count(&(wmap[sj]));
      if (ej < WH_BKT_NR) { // has space at home location
        wmap[sj].t[ej] = wmap[si].t[ei];
        pmap[sj].e[ej] = pmap[si].e[ei];
        const u32 ni = ci - 1;
        if (ei < ni) {
          wmap[si].t[ei] = wmap[si].t[ni];
          pmap[si].e[ei] = pmap[si].e[ni];
        }
        wmap[si].t[ni] = 0;
        pmap[si].e[ni] = NULL;
        ci--;
      }
    }
  }
}

  static void
wormhmap_expand(struct wormhmap * const hmap)
{
  // sync expand
  const u32 mask0 = hmap->mask;
  if (mask0 == UINT32_MAX)
    debug_die();
  const u32 nr0 = mask0 + 1;
  const u32 mask1 = mask0 + nr0;
  const u64 nr1 = ((u64)nr0) << 1; // must use u64; u32 can overflow
  const u64 wsize = nr1 * sizeof(hmap->wmap[0]);
  const u64 psize = nr1 * sizeof(hmap->pmap[0]);
  u64 msize = wsize + psize;
  u8 * mem = pages_alloc_best(msize, true, &msize);
  if (mem == NULL) {
    // We are at a very deep call stack from wormhole_put().
    // Gracefully handling the failure requires lots of changes.
    // Currently we simply wait for available memory
    // TODO: gracefully return with insertion failure
    char ts[64];
    time_stamp(ts, 64);
    fprintf(stderr, "%s %s sleep-wait for memory allocation %lukB\n",
        __func__, ts, msize >> 10);
    do {
      sleep(1);
      mem = pages_alloc_best(msize, true, &msize);
    } while (mem == NULL);
    time_stamp(ts, 64);
    fprintf(stderr, "%s %s memory allocation done\n", __func__, ts);
  }

  struct wormhmap hmap1 = *hmap;
  hmap1.pmap = (typeof(hmap1.pmap))mem;
  hmap1.wmap = (typeof(hmap1.wmap))(mem + psize);
  hmap1.msize = msize;
  hmap1.mask = mask1;

  const struct wormslot * const wmap0 = hmap->wmap;
  const struct wormmbkt * const pmap0 = hmap->pmap;

  for (u32 s = 0; s < nr0; s++) {
    const struct wormmbkt * const bkt = &pmap0[s];
    for (u32 i = 0; (i < WH_BKT_NR) && bkt->e[i]; i++) {
      const struct wormmeta * const meta = bkt->e[i];
      const u32 hash32 = wormmeta_hash32_load(meta);
      const u32 idx0 = hash32 & mask0;
      const u32 idx1 = ((idx0 == s) ? hash32 : wormhole_bswap(hash32)) & mask1;

      const u32 n = wormhmap_slot_count(&(hmap1.wmap[idx1]));
      debug_assert(n < 8);
      hmap1.wmap[idx1].t[n] = wmap0[s].t[i];
      hmap1.pmap[idx1].e[n] = bkt->e[i];
    }
  }
  pages_unmap(hmap->pmap, hmap->msize);
  hmap->pmap = hmap1.pmap;
  hmap->wmap = hmap1.wmap;
  hmap->msize = hmap1.msize;
  hmap->mask = hmap1.mask;
  wormhmap_squeeze(hmap);
}

  static bool
wormhmap_cuckoo(struct wormhmap * const hmap, const u32 mid0,
    struct wormmeta * const e0, const u16 s0, const u32 depth)
{
  const u32 ii = wormhmap_slot_count(&(hmap->wmap[mid0]));
  if (ii < WH_BKT_NR) {
    hmap->wmap[mid0].t[ii] = s0;
    hmap->pmap[mid0].e[ii] = e0;
    return true;
  } else if (depth == 0) {
    return false;
  }

  // depth > 0
  struct wormmbkt * const bkt = &(hmap->pmap[mid0]);
  u16 * const sv = &(hmap->wmap[mid0].t[0]);
  for (u32 i = 0; i < WH_BKT_NR; i++) {
    const struct wormmeta * const meta = bkt->e[i];
    debug_assert(meta);
    const u32 hash32 = wormmeta_hash32_load(meta);

    const u32 midx = hash32 & hmap->mask;
    const u32 midy = wormhole_bswap(hash32) & hmap->mask;
    const u32 midt = (midx != mid0) ? midx : midy;
    if (midt != mid0) { // possible
      // no penalty if moving someone back to its 1st hash location
      const u32 depth1 = (midt == midx) ? depth : (depth - 1);
      if (wormhmap_cuckoo(hmap, midt, bkt->e[i], sv[i], depth1)) {
        bkt->e[i] = e0;
        sv[i] = s0;
        return true;
      }
    }
  }
  return false;
}

  static void
wormhmap_set(struct wormhmap * const hmap, struct wormmeta * const meta)
{
  const u32 hash32 = wormmeta_hash32_load(meta);
  const u32 midx = hash32 & hmap->mask;
  const u32 midy = wormhole_bswap(hash32) & hmap->mask;
  const u16 pkey = wormhole_pkey(hash32);
  // insert with cuckoo
  if (likely(wormhmap_cuckoo(hmap, midx, meta, pkey, 1)))
    return;
  if (wormhmap_cuckoo(hmap, midy, meta, pkey, 1))
    return;
  if (wormhmap_cuckoo(hmap, midx, meta, pkey, 2))
    return;

  // expand
  wormhmap_expand(hmap);

  wormhmap_set(hmap, meta);
}

  static bool
wormhmap_del_slot(struct wormhmap * const hmap, const u32 mid,
    const struct wormmeta * const meta, const m128 skey)
{
  u32 mask = wormhmap_match_mask(&(hmap->wmap[mid]), skey);
  while (mask) {
    const u32 i2 = (u32)__builtin_ctz(mask);
    const struct wormmeta * const meta1 = hmap->pmap[mid].e[i2>>1];
    if (likely(meta == meta1)) {
      const u32 i = i2 >> 1;
      const u32 j = wormhmap_slot_count(&(hmap->wmap[mid])) - 1;
      hmap->wmap[mid].t[i] = hmap->wmap[mid].t[j];
      hmap->pmap[mid].e[i] = hmap->pmap[mid].e[j];
      hmap->wmap[mid].t[j] = 0;
      hmap->pmap[mid].e[j] = NULL;
      return true;
    }
    mask -= (3u << i2);
  }
  return false;
}

  static bool
wormhmap_del(struct wormhmap * const hmap, const struct wormmeta * const meta)
{
  const u32 hash32 = wormmeta_hash32_load(meta);
  const u32 midx = hash32 & hmap->mask;
  const u32 midy = wormhole_bswap(hash32) & hmap->mask;
  const m128 skey = wormhmap_m128_pkey(wormhole_pkey(hash32));
  return wormhmap_del_slot(hmap, midx, meta, skey)
    || wormhmap_del_slot(hmap, midy, meta, skey);
}

  static bool
wormhmap_replace_slot(struct wormhmap * const hmap, const u32 mid,
    const struct wormmeta * const old, const m128 skey, struct wormmeta * const new)
{
  u32 mask = wormhmap_match_mask(&(hmap->wmap[mid]), skey);
  while (mask) {
    const u32 i2 = (u32)__builtin_ctz(mask);
    struct wormmeta ** const pslot = &hmap->pmap[mid].e[i2>>1];
    if (likely(old == *pslot)) {
      *pslot = new;
      return true;
    }
    mask -= (3u << i2);
  }
  return false;
}

  static bool
wormhmap_replace(struct wormhmap * const hmap, const struct wormmeta * const old, struct wormmeta * const new)
{
  const u32 hash32 = wormmeta_hash32_load(old);
  const u32 midx = hash32 & hmap->mask;
  const u32 midy = wormhole_bswap(hash32) & hmap->mask;
  const m128 skey = wormhmap_m128_pkey(wormhole_pkey(hash32));
  return wormhmap_replace_slot(hmap, midx, old, skey, new)
    || wormhmap_replace_slot(hmap, midy, old, skey, new);
}
// }}} hmap

// create {{{
// it's unsafe
  static bool
wormhole_create_leaf0(struct wormhole_int * const map)
{
  const bool sr = wormhole_slab_reserve(map, 1);
  if (unlikely(!sr))
    return false;

  // create leaf of empty key
  struct kv * const anchor = wormhole_alloc_akey(0);
  if (anchor == NULL)
    return false;
  kv_dup2(kv_null(), anchor);

  struct wormleaf_int * const leaf0 = wormleaf_int_alloc(map, NULL, NULL, anchor);
  if (leaf0 == NULL) {
    wormhole_free_akey(anchor);
    return false;
  }

  struct kv * const mkey = wormhole_alloc_mkey(0);
  if (mkey == NULL) {
    wormleaf_int_free(map->slab_leaf, leaf0);
    return false;
  }

  wormhole_prefix(mkey, 0);
  mkey->refcnt = 0;
  // create meta of empty key
  for (u32 i = 0; i < 2; i++) {
    if (map->hmap2[i].slab1) {
      struct wormmeta * const m0 = wormmeta_alloc(&map->hmap2[i], leaf0, mkey, 0, WH_FO);
      debug_assert(m0); // already reserved enough
      wormhmap_set(&(map->hmap2[i]), m0);
    }
  }

  map->leaf0 = leaf0;
  return true;
}

  static struct wormhole_int *
wormhole_create_internal(const struct kvmap_mm * const mm, const u32 nh)
{
  struct wormhole_int * const map = yalloc(sizeof(*map));
  if (map == NULL)
    return NULL;
  memset(map, 0, sizeof(*map));
  // mm
  map->mm = mm ? (*mm) : kvmap_mm_dup;

  // pbuf for meta-merge
  map->pbuf = yalloc(1lu << 16); // 64kB
  if (map->pbuf == NULL)
    goto fail;

  // hmap
  for (u32 i = 0; i < nh; i++) {
    struct wormhmap * const hmap = &map->hmap2[i];
    if (!wormhmap_init(hmap, map->pbuf))
      goto fail;

    hmap->slab1 = slab_create(sizeof(struct wormmeta), WH_SLABMETA_SIZE);
    if (hmap->slab1 == NULL)
      goto fail;

    hmap->slab2 = slab_create(sizeof(struct wormmeta) + (sizeof(u64) * WH_BMNR), WH_SLABMETA_SIZE);
    if (hmap->slab2 == NULL)
      goto fail;
  }

  // leaf slab
  map->slab_leaf = slab_create(sizeof(struct wormleaf_int), WH_SLABLEAF_SIZE);
  if (map->slab_leaf == NULL)
    goto fail;

  // qsbr
  map->qsbr = qsbr_create();
  if (map->qsbr == NULL)
    goto fail;

  // leaf0
  if (!wormhole_create_leaf0(map))
    goto fail;

  rwlock_init(&(map->metalock));
  wormhmap_store(map, &map->hmap2[0]);
  return map;

fail:
  if (map->qsbr)
    qsbr_destroy(map->qsbr);

  if (map->slab_leaf)
    slab_destroy(map->slab_leaf);

  for (u32 i = 0; i < nh; i++) {
    struct wormhmap * const hmap = &map->hmap2[i];
    if (hmap->slab1)
      slab_destroy(hmap->slab1);
    if (hmap->slab2)
      slab_destroy(hmap->slab2);
    wormhmap_deinit(hmap);
  }

  if (map->pbuf)
    free(map->pbuf);

  free(map);
  return NULL;
}

  struct wormhole_int *
wormhole_int_create(const struct kvmap_mm * const mm)
{
  return wormhole_create_internal(mm, 2);
}

  struct wormhole_int *
whunsafe_int_create(const struct kvmap_mm * const mm)
{
  return wormhole_create_internal(mm, 1);
}
// }}} create

// jump {{{

// lcp {{{
// search in the hash table for the Longest Prefix Match of the search key
// The corresponding wormmeta node is returned and the LPM is recorded in kref
  static struct wormmeta *
wormhole_meta_lcp(const struct wormhmap * const hmap, struct kref * const kref, const u32 klen)
{
  // invariant: lo <= lcp < (lo + gd)
  // ending condition: gd == 1
  u32 gd = (hmap->maxplen < klen ? hmap->maxplen : klen) + 1u;
  u32 lo = 0;
  u32 loh = KV_CRC32C_SEED;

#define META_LCP_GAP_1 ((7u))
  while (META_LCP_GAP_1 < gd) {
    const u32 inc = gd >> 3 << 2; // x4
    const u32 hash32 = crc32c_inc_x4(kref->ptr + lo, inc, loh);
    if (wormhmap_peek(hmap, hash32)) {
      loh = hash32;
      lo += inc;
      gd -= inc;
    } else {
      gd = inc;
    }
  }

  while (1 < gd) {
    const u32 inc = gd >> 1;
    const u32 hash32 = crc32c_inc_123(kref->ptr + lo, inc, loh);
    if (wormhmap_peek(hmap, hash32)) {
      loh = hash32;
      lo += inc;
      gd -= inc;
    } else {
      gd = inc;
    }
  }
#undef META_LCP_GAP_1

  kref->hash32 = loh;
  kref->len = lo;
  struct wormmeta * ret = wormhmap_get_kref(hmap, kref);
  if (likely(ret != NULL))
    return ret;

  gd = lo;
  lo = 0;
  loh = KV_CRC32C_SEED;

#define META_LCP_GAP_2 ((5u))
  while (META_LCP_GAP_2 < gd) {
    const u32 inc = (gd * 3) >> 2;
    wormhole_kref_inc(kref, lo, loh, inc);
    struct wormmeta * const tmp = wormhmap_get_kref(hmap, kref);
    if (tmp) {
      loh = kref->hash32;
      lo += inc;
      gd -= inc;
      ret = tmp;
      if (wormmeta_bm_test(tmp, kref->ptr[lo])) {
        loh = crc32c_u8(loh, kref->ptr[lo]);
        lo++;
        gd--;
        ret = NULL;
      } else {
        gd = 1;
        break;
      }
    } else {
      gd = inc;
    }
  }

  while (1 < gd) {
    const u32 inc = (gd * 3) >> 2;
    wormhole_kref_inc_123(kref, lo, loh, inc);
    struct wormmeta * const tmp = wormhmap_get_kref(hmap, kref);
    if (tmp) {
      loh = kref->hash32;
      lo += inc;
      gd -= inc;
      ret = tmp;
      if (wormmeta_bm_test(tmp, kref->ptr[lo])) {
        loh = crc32c_u8(loh, kref->ptr[lo]);
        lo++;
        gd--;
        ret = NULL;
      } else {
        break;
      }
    } else {
      gd = inc;
    }
  }
#undef META_LCP_GAP_2

  if (kref->len != lo) {
    kref->hash32 = loh;
    kref->len = lo;
  }
  if (ret == NULL)
    ret = wormhmap_get_kref(hmap, kref);
  debug_assert(ret);
  return ret;
}
// }}} lcp

// down {{{
  static struct wormleaf_int *
wormhole_meta_down(const struct wormhmap * const hmap, const struct kref * const lcp,
    const struct wormmeta * const meta, const u32 klen)
{
  if (likely(lcp->len < klen)) { // partial match
    const u32 id0 = lcp->ptr[lcp->len];
    if (wormmeta_bitmin_load(meta) > id0) { // no left, don't care about right.
      return wormmeta_lpath_load(meta);
    } else if (wormmeta_bitmax_load(meta) < id0) { // has left sibling but no right sibling
      return wormmeta_rmost_load(meta);
    } else { // has both (expensive)
      return wormmeta_rmost_load(wormhmap_get_kref1(hmap, lcp, (u8)wormmeta_bm_lt(meta, id0)));
    }
  } else { // lcp->len == klen
    return wormmeta_lpath_load(meta);
  }
}
// }}} down

// jump-rw {{{
  static struct wormleaf_int *
wormhole_jump_leaf(const struct wormhmap * const hmap, const struct kref * const key)
{
  struct kref kref = {.ptr = key->ptr};
  debug_assert(kv_crc32c(key->ptr, key->len) == key->hash32);

  const struct wormmeta * const meta = wormhole_meta_lcp(hmap, &kref, key->len);
  return wormhole_meta_down(hmap, &kref, meta, key->len);
}

  static struct wormleaf_int *
wormhole_jump_leaf_read(struct wormref_int * const ref, const struct kref * const key)
{
  struct wormhole_int * const map = ref->map;
#pragma nounroll
  do {
    const struct wormhmap * const hmap = wormhmap_load(map);
    const u64 v = wormhmap_version_load(hmap);
    qsbr_update(&ref->qref, v);
    struct wormleaf_int * const leaf = wormhole_jump_leaf(hmap, key);
#pragma nounroll
    do {
      if (rwlock_trylock_read_nr(&(leaf->leaflock), 64)) {
        if (wormleaf_int_version_load(leaf) <= v)
          return leaf;
        wormleaf_int_unlock_read(leaf);
        break;
      }
      // v1 is loaded before lv; if lv <= v, can update v1 without redo jump
      const u64 v1 = wormhmap_version_load(wormhmap_load(map));
      if (wormleaf_int_version_load(leaf) > v)
        break;
      wormhole_qsbr_update_pause(ref, v1);
    } while (true);
  } while (true);
}

  static struct wormleaf_int *
wormhole_jump_leaf_write(struct wormref_int * const ref, const struct kref * const key)
{
  struct wormhole_int * const map = ref->map;
#pragma nounroll
  do {
    const struct wormhmap * const hmap = wormhmap_load(map);
    const u64 v = wormhmap_version_load(hmap);
    qsbr_update(&ref->qref, v);
    struct wormleaf_int * const leaf = wormhole_jump_leaf(hmap, key);
    wormleaf_int_prefetch(leaf, key->hash32);
#pragma nounroll
    do {
      if (rwlock_trylock_write_nr(&(leaf->leaflock), 64)) {
        if (wormleaf_int_version_load(leaf) <= v)
          return leaf;
        wormleaf_int_unlock_write(leaf);
        break;
      }
      // v1 is loaded before lv; if lv <= v, can update v1 without redo jump
      const u64 v1 = wormhmap_version_load(wormhmap_load(map));
      if (wormleaf_int_version_load(leaf) > v)
        break;
      wormhole_qsbr_update_pause(ref, v1);
    } while (true);
  } while (true);
}
// }}} jump-rw

// }}} jump

// leaf-read {{{
// leaf must have been sorted
// return the key at [i] as if k1 has been inserted into leaf; i <= leaf->nr_sorted
  static const struct int_store_pair *
wormleaf_kv_at_is1(const struct wormleaf_int * const leaf, const u32 i, const u32 is1,
                   const struct int_store_pair * const k1)
{
    if (i != is1)
        return leaf->kvs + i - (i > is1 ? 1 : 0);
    return k1;
}

  static u32
wormleaf_int_search_eq(const struct wormleaf_int * const leaf, const struct kref * const key)
{
  const u64 search_key = _bswap64((*((u64 *) key->ptr)) & BITMASK(key->len * 8));
  u32 lo = -1;
  u32 hi = leaf->nr_sorted;
  while (hi - lo > 1) {
    const u32 i = (lo + hi) >> 1;
    const struct int_store_pair * const curr = leaf->kvs + i;
    const int cmp = compare_int_isp(search_key, key->len, curr);
    lo = cmp <= 0 ? lo : i;
    hi = cmp <= 0 ? i : hi;
  }
  u32 res = (hi < leaf->nr_keys && compare_int_isp(search_key, key->len, leaf->kvs + hi) == 0) ? hi : WH_KPN;
  return res;
}


// search the first key that is >= the given key
// return 0 .. nr_sorted
  static u32
wormleaf_int_search(const struct wormleaf_int * const leaf, const struct kref * const key)
{
  if (key->ptr == NULL)
    return 0;
  u64 search_key = _bswap64((*((u64 *) key->ptr)) & BITMASK(key->len * 8));
  int lo = -1;
  int hi = leaf->nr_sorted;
  while (hi - lo > 1) {
    const int i = (lo + hi) >> 1;
    const struct int_store_pair * const curr = leaf->kvs + i;
    const int cmp = compare_int_isp(search_key, key->len, curr);
    lo = cmp <= 0 ? lo : i;
    hi = cmp <= 0 ? i : hi;
  }
  return hi;
}

  static u32
wormleaf_int_seek(const struct wormleaf_int * const leaf, const struct kref * const key)
{
  debug_assert(leaf->nr_sorted == leaf->nr_keys);
  return wormleaf_int_search(leaf, key);
}

// same to search_sorted but the target is very likely beyond the end
  static u32
wormleaf_int_seek_end(const struct wormleaf_int * const leaf, const struct kref * const key)
{
  u64 search_key = _bswap64((*((u64 *) key->ptr)) & BITMASK(key->len * 8));
  debug_assert(leaf->nr_keys == leaf->nr_sorted);
  if (leaf->nr_sorted) {
    const int cmp = compare_int_isp(search_key, key->len, leaf->kvs + leaf->nr_sorted - 1);
    if (cmp > 0)
      return leaf->nr_sorted;
    else if (cmp == 0)
      return leaf->nr_sorted - 1;
    else
      return wormleaf_int_seek(leaf, key);
  } else {
    return 0;
  }
}
// }}} leaf-read

// leaf-write {{{
  static void
wormleaf_int_sort_m2(struct wormleaf_int * const leaf, const u32 n1, const u32 n2)
{
  if (n1 == 0 || n2 == 0)
    return; // no need to sort

  struct int_store_pair * const kvs = leaf->kvs;
  struct int_store_pair et[WH_KPN / 2]; // min(n1,n2) < KPN / 2
  if (n1 <= n2) { // merge left
    memcpy(et, &(kvs[0]), sizeof(kvs[0]) * n1);
    struct int_store_pair * eo = kvs;
    struct int_store_pair * e1 = et; // size == n1
    struct int_store_pair * e2 = &(kvs[n1]); // size == n2
    const struct int_store_pair * const z1 = e1 + n1;
    const struct int_store_pair * const z2 = e2 + n2;
    while ((e1 < z1) && (e2 < z2)) {
      const int cmp = compare_isp_isp(e1, e2);
      if (cmp < 0)
        *(eo++) = *(e1++);
      else if (cmp > 0)
        *(eo++) = *(e2++);
      else
        debug_die();

      if (eo == e2)
        break; // finish early
    }
    if (eo < e2)
      memcpy(eo, e1, sizeof(*eo) * (size_t)(e2 - eo));
  } else {
    memcpy(et, &(kvs[n1]), sizeof(kvs[0]) * n2);
    struct int_store_pair * eo = &(kvs[n1 + n2 - 1]); // merge backwards
    struct int_store_pair * e1 = &(kvs[n1 - 1]); // size == n1
    struct int_store_pair * e2 = &(et[n2 - 1]); // size == n2
    const struct int_store_pair * const z1 = e1 - n1;
    const struct int_store_pair * const z2 = e2 - n2;
    while ((e1 > z1) && (e2 > z2)) {
      const int cmp = compare_isp_isp(e1, e2);
      if (cmp < 0)
        *(eo--) = *(e2--);
      else if (cmp > 0)
        *(eo--) = *(e1--);
      else
        debug_die();

      if (eo == e1)
        break;
    }
    if (eo > e1)
      memcpy(e1 + 1, et, sizeof(*eo) * (size_t)(eo - e1));
  }
}

  static int
wormleaf_int_cmp(const void * const p1, const void * const p2, void * priv)
{
  const struct int_store_pair * const k1 = (const struct int_store_pair *) p1;
  const struct int_store_pair * const k2 = (const struct int_store_pair *) p2;
  return compare_isp_isp(p1, p2);
}

  static inline void
wormleaf_sort_range(struct wormleaf_int * const leaf, const u32 i0, const u32 nr)
{
  qsort_r(&(leaf->kvs[i0]), nr, sizeof(leaf->kvs[i0]), wormleaf_int_cmp, leaf);
}

// make sure all keys are sorted in a leaf node
  static void
wormleaf_int_sync_sorted(struct wormleaf_int * const leaf)
{
  const u32 s = leaf->nr_sorted;
  const u32 n = leaf->nr_keys;
  if (s == n)
    return;

  wormleaf_sort_range(leaf, s, n - s);
  // merge-sort inplace
  wormleaf_int_sort_m2(leaf, s, n - s);
  leaf->nr_sorted = n;
}

// shift a sequence of entries on hs and update the corresponding ss values
  static void
wormleaf_int_shift_inc(struct wormleaf_int * const leaf, const u32 to, const u32 from, const u32 nr)
{
  debug_assert(to == (from+1));
  memmove(&(leaf->kvs[to]), &(leaf->kvs[from]), sizeof(leaf->kvs[from]) * nr);
}

  static void
wormleaf_int_shift_dec(struct wormleaf_int * const leaf, const u32 to, const u32 from, const u32 nr)
{
  debug_assert(to == (from-1));
  memmove(&(leaf->kvs[to]), &(leaf->kvs[from]), sizeof(leaf->kvs[from]) * nr);
}

  static void
wormleaf_int_insert(struct wormleaf_int * const leaf, const struct kv * const new)
{
  debug_assert(new->hash == kv_crc32c_extend(kv_crc32c(new->kv, new->klen)));
  debug_assert(leaf->nr_keys < WH_KPN);

  // insert
  const u32 nr0 = leaf->nr_keys;
  leaf->kvs[nr0].key_size = new->klen;
  leaf->kvs[nr0].key = (*((u64 *) new->kv)) & BITMASK(new->klen * 8);
  memcpy(leaf->kvs[nr0].store, new->kv + new->klen, new->vlen);
  leaf->nr_keys++;

  // optimize for seq insertion
  if (nr0 == leaf->nr_sorted) {
    if (nr0) {
      if (compare_isp_isp(leaf->kvs + nr0 - 1, leaf->kvs + nr0) <= 0)
        leaf->nr_sorted = nr0 + 1;
    } else {
      leaf->nr_sorted = 1;
    }
  }
}

  static void
wormleaf_int_insert_isp(struct wormleaf_int * const leaf, const struct int_store_pair * const new)
{
  debug_assert(new->hash == kv_crc32c_extend(kv_crc32c(new->kv, new->klen)));
  debug_assert(leaf->nr_keys < WH_KPN);

  // insert
  const u32 nr0 = leaf->nr_keys;
  leaf->kvs[nr0] = *new;
  leaf->nr_keys++;

  // optimize for seq insertion
  if (nr0 == leaf->nr_sorted) {
    if (nr0) {
      if (compare_isp_isp(leaf->kvs + nr0 - 1, leaf->kvs + nr0) <= 0)
        leaf->nr_sorted = nr0 + 1;
    } else {
      leaf->nr_sorted = 1;
    }
  }
}

// internal only
  static void
wormleaf_int_remove(struct wormleaf_int * const leaf, const u32 pos)
{
  // ss
  leaf->kvs[pos] = leaf->kvs[leaf->nr_keys - 1];
  if (leaf->nr_sorted > pos)
    leaf->nr_sorted = pos;
  leaf->nr_keys--;
}

// for delr (delete-range)
  static void
wormleaf_int_delete_range(struct wormleaf_int * const leaf, const u32 i0, const u32 end)
{
  debug_assert(leaf->nr_keys == leaf->nr_sorted);
  for (u32 i = end; i > i0; i--) {
    const u32 ir = i - 1;
    wormleaf_int_remove(leaf, ir);
  }
}

// return the old kv; the caller should free the old kv
  static void
wormleaf_int_update(struct wormleaf_int * const leaf, const u32 ih, const struct kv * const new)
{
  debug_assert(new->hash == kv_crc32c_extend(kv_crc32c(new->kv, new->klen)));
  leaf->kvs[ih].key_size = new->klen;
  leaf->kvs[ih].key = (*((u64 *) new->kv)) & BITMASK(new->klen * 8);
  memcpy(leaf->kvs[ih].store, new->kv + new->klen, new->vlen);
}
// }}} leaf-write

// leaf-split {{{
// It only works correctly in cut_search
// quickly tell if a cut between k1 and k2 can achieve a specific anchor-key length
  static bool
wormhole_split_cut_alen_check(const u32 alen, const struct kv * const k1, const struct kv * const k2)
{
  debug_assert(k2->klen >= alen);
  return (k1->klen < alen) || (k1->kv[alen - 1] != k2->kv[alen - 1]);
}

  static bool
wormhole_isp_split_cut_alen_check(const u32 alen, const struct int_store_pair * const k1,
                                  const struct int_store_pair * const k2)
{
  debug_assert(k2->key_size >= alen);
  return (k1->key_size < alen) || (((k1->key >> (56 - (alen - 1) * 8)) & ((1u << 8) - 1)) 
                                        != ((k2->key >> (56 - (alen - 1) * 8)) & ((1u << 8) - 1)));
}

// return the number of keys that should go to leaf1
// assert(r > 0 && r <= nr_keys)
// (1) r < is1, anchor key is ss[r-1]:ss[r]
// (2) r == is1: anchor key is ss[r-1]:new
// (3) r == is1+1: anchor key is new:ss[r-1] (ss[r-1] is the ss[r] on the logically sorted array)
// (4) r > is1+1: anchor key is ss[r-2]:ss[r-1] (ss[r-2] is the [r-1] on the logically sorted array)
// edge cases:
//   (case 2) is1 == nr_keys: r = nr_keys; ss[r-1]:new
//   (case 3) is1 == 0, r == 1; new:ss[0]
// return 1..WH_KPN
  static u32
wormhole_split_cut_search1(struct wormleaf_int * const leaf, u32 l, u32 h, const u32 is1,
                           const struct int_store_pair * const new)
{
  debug_assert(leaf->nr_keys == leaf->nr_sorted);
  debug_assert(leaf->nr_keys);
  debug_assert(l < h && h <= leaf->nr_sorted);

  const struct int_store_pair * const kl0 = wormleaf_kv_at_is1(leaf, l, is1, new);
  const struct int_store_pair * const kh0 = wormleaf_kv_at_is1(leaf, h, is1, new);
  const u32 alen = isp_key_lcp(kl0, kh0) + 1;
  if (unlikely(alen > UINT16_MAX))
    return WH_KPN2;

  const u32 target = leaf->next ? WH_MID : WH_KPN_MRG;
  while ((l + 1) < h) {
    const u32 m = (l + h + 1) >> 1;
    if (m <= target) { // try right
      const struct int_store_pair * const k1 = wormleaf_kv_at_is1(leaf, m, is1, new);
      const struct int_store_pair * const k2 = wormleaf_kv_at_is1(leaf, h, is1, new);
      if (wormhole_isp_split_cut_alen_check(alen, k1, k2))
        l = m;
      else
        h = m;
    } else { // try left
      const struct int_store_pair * const k1 = wormleaf_kv_at_is1(leaf, l, is1, new);
      const struct int_store_pair * const k2 = wormleaf_kv_at_is1(leaf, m, is1, new);
      if (wormhole_isp_split_cut_alen_check(alen, k1, k2))
        h = m;
      else
        l = m;
    }
  }
  return h;
}

  static void
wormhole_split_leaf_move1(struct wormleaf_int * const leaf1, struct wormleaf_int * const leaf2,
    const u32 cut, const u32 is1, const struct kv * const new)
{
  const u32 nr_keys = leaf1->nr_keys;
  struct int_store_pair new_isp;
  new_isp.key_size = new->klen;
  new_isp.key = (*((u64 *) new->kv)) & BITMASK(8 * new->klen);
  memcpy(new_isp.store, new->kv + new->klen, new->vlen);
  struct int_store_pair es[WH_KPN];

  if (cut <= is1) { // new_isp goes to leaf2
    // leaf2
    for (u32 i = cut; i < is1; i++)
      wormleaf_int_insert_isp(leaf2, leaf1->kvs + i);
    wormleaf_int_insert_isp(leaf2, &new_isp);
    for (u32 i = is1; i < nr_keys; i++)
      wormleaf_int_insert_isp(leaf2, leaf1->kvs + i);

    // leaf1
    for (u32 i = 0; i < cut; i++)
      es[i] = leaf1->kvs[i];
  } else { // new_isp goes to leaf1
    // leaf2
    for (u32 i = cut - 1; i < nr_keys; i++)
      wormleaf_int_insert_isp(leaf2, leaf1->kvs + i);

    // leaf1
    for (u32 i = 0; i < is1; i++)
      es[i] = leaf1->kvs[i];
    es[is1] = new_isp;
    for (u32 i = is1 + 1; i < cut; i++)
      es[i] = leaf1->kvs[i - 1];
  }

  leaf2->nr_sorted = leaf2->nr_keys;

  memset(leaf1->kvs, 0, sizeof(leaf1->kvs[0]) * WH_KPN);
  leaf1->nr_keys = 0;
  for (u32 i = 0; i < cut; i++)
    wormleaf_int_insert_isp(leaf1, es + i);
  leaf1->nr_sorted = cut;
  debug_assert((leaf1->nr_sorted + leaf2->nr_sorted) == (nr_keys + 1));
}

// create an anchor for leaf-split
  static struct kv *
wormhole_split_alloc_anchor(const struct kv * const key1, const struct kv * const key2)
{
  const u32 alen = kv_key_lcp(key1, key2) + 1;
  debug_assert(alen <= key2->klen);

  struct kv * const anchor = wormhole_alloc_akey(alen);
  if (anchor)
    kv_refill(anchor, key2->kv, alen, NULL, 0);
  return anchor;
}

// create an anchor for leaf-split
  static struct kv *
wormhole_isp_split_alloc_anchor(const struct int_store_pair * const key1, const struct int_store_pair * const key2)
{
  const u32 alen = isp_key_lcp(key1, key2) + 1;
  debug_assert(alen <= key2->klen);

  struct kv * const anchor = wormhole_alloc_akey(alen);
  if (anchor) {
    u8 buf[key2->key_size + sizeof(key2->store)];
    memcpy(buf, &key2->key, key2->key_size);
    memcpy(buf + key2->key_size, key2->store, sizeof(key2->store));
    kv_refill(anchor, buf, alen, NULL, 0);
  }
  return anchor;
}

// leaf1 is locked
// split leaf1 into leaf1+leaf2; insert new into leaf1 or leaf2, return leaf2
  static struct wormleaf_int *
wormhole_split_leaf(struct wormhole_int * const map, struct wormleaf_int * const leaf1, struct kv * const new)
{
  struct int_store_pair new_isp;
  new_isp.key_size = new->klen;
  new_isp.key = (*((u64 *) new->kv)) & BITMASK(8 * new->klen);
  memcpy(new_isp.store, new->kv + new->klen, new->vlen);

  wormleaf_int_sync_sorted(leaf1);
  struct kref kref_new;
  kref_ref_kv(&kref_new, new);
  const u32 is1 = wormleaf_int_search(leaf1, &kref_new); // new should be inserted at [is1]
  const u32 cut = wormhole_split_cut_search1(leaf1, 0, leaf1->nr_keys, is1, &new_isp);
  if (unlikely(cut == WH_KPN2))
    return NULL;

  // anchor of leaf2
  debug_assert(cut && (cut <= leaf1->nr_keys));
  const struct int_store_pair * const key1 = wormleaf_kv_at_is1(leaf1, cut - 1, is1, &new_isp);
  const struct int_store_pair * const key2 = wormleaf_kv_at_is1(leaf1, cut, is1, &new_isp);
  struct kv * const anchor2 = wormhole_isp_split_alloc_anchor(key1, key2);
  if (unlikely(anchor2 == NULL)) // anchor alloc failed
    return NULL;

  // create leaf2 with anchor2
  struct wormleaf_int * const leaf2 = wormleaf_int_alloc(map, leaf1, leaf1->next, anchor2);
  if (unlikely(leaf2 == NULL)) {
    wormhole_free_akey(anchor2);
    return NULL;
  }

  // split_hmap will unlock the leaf nodes; must move now
  wormhole_split_leaf_move1(leaf1, leaf2, cut, is1, new);
  // leaf1 and leaf2 should be sorted after split
  debug_assert(leaf1->nr_keys == leaf1->nr_sorted);
  debug_assert(leaf2->nr_keys == leaf2->nr_sorted);

  return leaf2;
}
// }}} leaf-split

// leaf-merge {{{
// MERGE is the only operation that deletes a leaf node (leaf2).
// It ALWAYS merges the right node into the left node even if the left is empty.
// This requires both of their writer locks to be acquired.
// This allows iterators to safely probe the next node (but not backwards).
// In other words, if either the reader or the writer lock of node X has been acquired:
// X->next (the pointer) cannot be changed by any other thread.
// X->next cannot be deleted.
// But the content in X->next can still be changed.
  static bool
wormleaf_merge(struct wormleaf_int * const leaf1, struct wormleaf_int * const leaf2)
{
  debug_assert((leaf1->nr_keys + leaf2->nr_keys) <= WH_KPN);
  const bool leaf1_sorted = leaf1->nr_keys == leaf1->nr_sorted;

  for (u32 i = 0; i < leaf2->nr_keys; i++)
    wormleaf_int_insert_isp(leaf1, leaf2->kvs + i);
  if (leaf1_sorted)
    leaf1->nr_sorted += leaf2->nr_sorted;
  return true;
}

// for undoing insertion under split_meta failure; leaf2 is still local
// remove the new key; merge keys in leaf2 into leaf1; free leaf2
  static void
wormleaf_split_undo(struct wormhole_int * const map, struct wormleaf_int * const leaf1,
    struct wormleaf_int * const leaf2, struct kv * const new)
{
  if (new) {
    struct kref new_kref;
    kref_ref_kv(&new_kref, new);

    const u32 im1 = wormleaf_int_search_eq(leaf1, &new_kref);
    if (im1 < WH_KPN) {
      wormleaf_int_remove(leaf1, im1);
    } else { // not found in leaf1; search leaf2
      const u32 im2 = wormleaf_int_search_eq(leaf2, &new_kref);
      debug_assert(im2 < WH_KPN);
      wormleaf_int_remove(leaf2, im2);
    }
  }
  // this merge must succeed
  if (!wormleaf_merge(leaf1, leaf2))
    debug_die();
  // Keep this to avoid triggering false alarm in wormleaf_free
  leaf2->leaflock.opaque = 0;
  wormleaf_int_free(map->slab_leaf, leaf2);
}
// }}} leaf-merge

// get/probe {{{
  struct kv *
wormhole_int_get(struct wormref_int * const ref, const struct kref * const key, struct kv * const out)
{
  struct wormleaf_int * const leaf = wormhole_jump_leaf_read(ref, key);
  const u32 i = wormleaf_int_search_eq(leaf, key);
  if (i >= WH_KPN) {
      wormleaf_int_unlock_read(leaf);
      return NULL;
  }

  struct int_store_pair * const res = leaf->kvs + i;
  out->klen = res->key_size;
  memcpy(out->kv, &res->key, out->klen);
  out->vlen = sizeof(res->store);
  memcpy(out->kv + out->klen, res->store, out->vlen);

  wormleaf_int_unlock_read(leaf);
  return out;
}

  struct kv *
whsafe_int_get(struct wormref_int * const ref, const struct kref * const key, struct kv * const out)
{
  wormhole_int_resume(ref);
  struct kv * const ret = wormhole_int_get(ref, key, out);
  wormhole_int_park(ref);
  return ret;
}

  struct kv *
whunsafe_int_get(struct wormhole_int * const map, const struct kref * const key, struct kv * const out)
{
  struct wormleaf_int * const leaf = wormhole_jump_leaf(map->hmap, key);
  const u32 i = wormleaf_int_search_eq(leaf, key);
  if (i >= WH_KPN)
      return NULL;

  struct int_store_pair * const res = leaf->kvs + i;
  out->klen = res->key_size;
  memcpy(out->kv, &res->key, out->klen);
  out->vlen = sizeof(res->store);
  memcpy(out->kv + out->klen, res->store, out->vlen);

  return out;
}

  bool
wormhole_int_probe(struct wormref_int * const ref, const struct kref * const key)
{
  struct wormleaf_int * const leaf = wormhole_jump_leaf_read(ref, key);
  const u32 i = wormleaf_int_search_eq(leaf, key);
  wormleaf_int_unlock_read(leaf);
  return i < WH_KPN;
}

  bool
whsafe_int_probe(struct wormref_int * const ref, const struct kref * const key)
{
  wormhole_int_resume(ref);
  const bool r = wormhole_int_probe(ref, key);
  wormhole_int_park(ref);
  return r;
}

  bool
whunsafe_int_probe(struct wormhole_int * const map, const struct kref * const key)
{
  struct wormleaf_int * const leaf = wormhole_jump_leaf(map->hmap, key);
  return wormleaf_int_search_eq(leaf, key) < WH_KPN;
}
// }}} get/probe

// meta-split {{{
// duplicate from meta1; only has one bit but will soon add a new bit
  static struct wormmeta *
wormmeta_expand(struct wormhmap * const hmap, struct wormmeta * const meta1)
{
  struct wormmeta * const meta2 = slab_alloc_unsafe(hmap->slab2);
  if (meta2 == NULL)
    return NULL;

  memcpy(meta2, meta1, sizeof(*meta1));
  for (u32 i = 0; i < WH_BMNR; i++)
    meta2->bitmap[i] = 0;
  const u32 bitmin = wormmeta_bitmin_load(meta1);
  debug_assert(bitmin == wormmeta_bitmax_load(meta1));
  debug_assert(bitmin < WH_FO);
  // set the only bit
  meta2->bitmap[bitmin >> 6u] |= (1lu << (bitmin & 0x3fu));

  wormhmap_replace(hmap, meta1, meta2);
  slab_free_unsafe(hmap->slab1, meta1);
  return meta2;
}

  static struct wormmeta *
wormmeta_bm_set_helper(struct wormhmap * const hmap, struct wormmeta * const meta, const u32 id)
{
  debug_assert(id < WH_FO);
  const u32 bitmin = wormmeta_bitmin_load(meta);
  const u32 bitmax = wormmeta_bitmax_load(meta);
  if (bitmin < bitmax) { // already in full size
    wormmeta_bm_set(meta, id);
    return meta;
  } else if (id == bitmin) { // do nothing
    return meta;
  } else if (bitmin == WH_FO) { // add the first bit
    wormmeta_bitmin_store(meta, id);
    wormmeta_bitmax_store(meta, id);
    return meta;
  } else { // need to expand
    struct wormmeta * const meta2 = wormmeta_expand(hmap, meta);
    wormmeta_bm_set(meta2, id);
    return meta2;
  }
}

// return true if a new node is created
  static void
wormmeta_split_touch(struct wormhmap * const hmap, struct kv * const mkey,
    struct wormleaf_int * const leaf, const u32 alen)
{
  struct wormmeta * meta = wormhmap_get(hmap, mkey);
  if (meta) {
    if (mkey->klen < alen)
      meta = wormmeta_bm_set_helper(hmap, meta, mkey->kv[mkey->klen]);
    if (wormmeta_lmost_load(meta) == leaf->next)
      wormmeta_lmost_store(meta, leaf);
    else if (wormmeta_rmost_load(meta) == leaf->prev)
      wormmeta_rmost_store(meta, leaf);
  } else { // create new node
    const u32 bit = (mkey->klen < alen) ? mkey->kv[mkey->klen] : WH_FO;
    meta = wormmeta_alloc(hmap, leaf, mkey, alen, bit);
    debug_assert(meta);
    wormhmap_set(hmap, meta);
  }
}

  static void
wormmeta_lpath_update(struct wormhmap * const hmap, const struct kv * const a1, const struct kv * const a2,
    struct wormleaf_int * const lpath)
{
  struct kv * const pbuf = hmap->pbuf;
  kv_dup2_key(a2, pbuf);

  // only need to update a2's own branch
  u32 i = kv_key_lcp(a1, a2) + 1;
  debug_assert(i <= pbuf->klen);
  wormhole_prefix(pbuf, i);
  while (i < a2->klen) {
    debug_assert(i <= hmap->maxplen);
    struct wormmeta * const meta = wormhmap_get(hmap, pbuf);
    debug_assert(meta);
    wormmeta_lpath_store(meta, lpath);

    i++;
    wormhole_prefix_inc1(pbuf);
  }
}

// for leaf1, a leaf2 is already linked at its right side.
// this function updates the meta-map by moving leaf1 and hooking leaf2 at correct positions
  static void
wormmeta_split(struct wormhmap * const hmap, struct wormleaf_int * const leaf,
    struct kv * const mkey)
{
  // left branches
  struct wormleaf_int * const prev = leaf->prev;
  struct wormleaf_int * const next = leaf->next;
  u32 i = next ? kv_key_lcp(prev->anchor, next->anchor) : 0;
  const u32 alen = leaf->anchor->klen;

  // save klen
  const u32 mklen = mkey->klen;
  wormhole_prefix(mkey, i);
  do {
    wormmeta_split_touch(hmap, mkey, leaf, alen);
    if (i >= alen)
      break;
    i++;
    wormhole_prefix_inc1(mkey);
  } while (true);

  // adjust maxplen; i is the plen of the last _touch()
  if (i > hmap->maxplen)
    hmap->maxplen = i;
  debug_assert(i <= UINT16_MAX);

  // restore klen
  mkey->klen = mklen;

  if (next)
    wormmeta_lpath_update(hmap, leaf->anchor, next->anchor, leaf);
}

// all locks will be released before returning
  static bool
wormhole_split_meta(struct wormref_int * const ref, struct wormleaf_int * const leaf2)
{
  struct kv * const mkey = wormhole_alloc_mkey(leaf2->anchor->klen);
  if (unlikely(mkey == NULL))
    return false;
  kv_dup2_key(leaf2->anchor, mkey);

  struct wormhole_int * const map = ref->map;
  // metalock
  wormhmap_lock(map, ref);

  // check slab reserve
  const bool sr = wormhole_slab_reserve(map, mkey->klen);
  if (unlikely(!sr)) {
    wormhmap_unlock(map);
    wormhole_free_mkey(mkey);
    return false;
  }

  struct wormhmap * const hmap0 = wormhmap_load(map);
  struct wormhmap * const hmap1 = wormhmap_switch(map, hmap0);

  // link
  struct wormleaf_int * const leaf1 = leaf2->prev;
  leaf1->next = leaf2;
  if (leaf2->next)
    leaf2->next->prev = leaf2;

  // update versions
  const u64 v1 = wormhmap_version_load(hmap0) + 1;
  wormleaf_int_version_store(leaf1, v1);
  wormleaf_int_version_store(leaf2, v1);
  wormhmap_version_store(hmap1, v1);

  wormmeta_split(hmap1, leaf2, mkey);

  qsbr_update(&ref->qref, v1);

  // switch hmap
  wormhmap_store(map, hmap1);

  wormleaf_int_unlock_write(leaf1);
  wormleaf_int_unlock_write(leaf2);

  qsbr_wait(map->qsbr, v1);

  wormmeta_split(hmap0, leaf2, mkey);

  wormhmap_unlock(map);

  if (mkey->refcnt == 0) // this is possible
    wormhole_free_mkey(mkey);
  return true;
}

// all locks (metalock + leaflocks) will be released before returning
// leaf1->lock (write) is already taken
  static bool
wormhole_split_insert(struct wormref_int * const ref, struct wormleaf_int * const leaf1,
    struct kv * const new)
{
  struct wormleaf_int * const leaf2 = wormhole_split_leaf(ref->map, leaf1, new);
  if (unlikely(leaf2 == NULL)) {
    wormleaf_int_unlock_write(leaf1);
    return false;
  }

  rwlock_lock_write(&(leaf2->leaflock));
  const bool rsm = wormhole_split_meta(ref, leaf2);
  if (unlikely(!rsm)) {
    // undo insertion & merge; free leaf2
    wormleaf_split_undo(ref->map, leaf1, leaf2, new);
    wormleaf_int_unlock_write(leaf1);
  }
  return rsm;
}

  static bool
whunsafe_split_meta(struct wormhole_int * const map, struct wormleaf_int * const leaf2)
{
  struct kv * const mkey = wormhole_alloc_mkey(leaf2->anchor->klen);
  if (unlikely(mkey == NULL))
    return false;
  kv_dup2_key(leaf2->anchor, mkey);

  const bool sr = wormhole_slab_reserve(map, mkey->klen);
  if (unlikely(!sr)) {
    wormhmap_unlock(map);
    wormhole_free_mkey(mkey);
    return false;
  }

  // link
  leaf2->prev->next = leaf2;
  if (leaf2->next)
    leaf2->next->prev = leaf2;

  for (u32 i = 0; i < 2; i++)
    if (map->hmap2[i].pmap)
      wormmeta_split(&(map->hmap2[i]), leaf2, mkey);
  if (mkey->refcnt == 0) // this is possible
    wormhole_free_mkey(mkey);
  return true;
}

  static bool
whunsafe_split_insert(struct wormhole_int * const map, struct wormleaf_int * const leaf1,
    struct kv * const new)
{
  struct wormleaf_int * const leaf2 = wormhole_split_leaf(map, leaf1, new);
  if (unlikely(leaf2 == NULL))
    return false;

  const bool rsm = whunsafe_split_meta(map, leaf2);
  if (unlikely(!rsm))  // undo insertion, merge, free leaf2
    wormleaf_split_undo(map, leaf1, leaf2, new);

  return rsm;
}
// }}} meta-split

// meta-merge {{{
// now it only contains one bit
  static struct wormmeta *
wormmeta_shrink(struct wormhmap * const hmap, struct wormmeta * const meta2)
{
  debug_assert(wormmeta_bitmin_load(meta2) == wormmeta_bitmax_load(meta2));
  struct wormmeta * const meta1 = slab_alloc_unsafe(hmap->slab1);
  if (meta1 == NULL)
    return NULL;

  memcpy(meta1, meta2, sizeof(*meta1));

  wormhmap_replace(hmap, meta2, meta1);
  slab_free_unsafe(hmap->slab2, meta2);
  return meta1;
}

  static void
wormmeta_bm_clear_helper(struct wormhmap * const hmap, struct wormmeta * const meta, const u32 id)
{
  if (wormmeta_bitmin_load(meta) == wormmeta_bitmax_load(meta)) {
    debug_assert(wormmeta_bitmin_load(meta) < WH_FO);
    wormmeta_bitmin_store(meta, WH_FO);
    wormmeta_bitmax_store(meta, WH_FO);
  } else { // has more than 1 bit
    wormmeta_bm_clear(meta, id);
    if (wormmeta_bitmin_load(meta) == wormmeta_bitmax_load(meta))
      wormmeta_shrink(hmap, meta);
  }
}

// all locks held
  static void
wormmeta_merge(struct wormhmap * const hmap, struct wormleaf_int * const leaf)
{
  // leaf->next is the new next after merge, which can be NULL
  struct wormleaf_int * const prev = leaf->prev;
  struct wormleaf_int * const next = leaf->next;
  struct kv * const pbuf = hmap->pbuf;
  kv_dup2_key(leaf->anchor, pbuf);
  u32 i = (prev && next) ? kv_key_lcp(prev->anchor, next->anchor) : 0;
  const u32 alen = leaf->anchor->klen;
  wormhole_prefix(pbuf, i);
  struct wormmeta * parent = NULL;
  do {
    debug_assert(i <= hmap->maxplen);
    struct wormmeta * meta = wormhmap_get(hmap, pbuf);
    if (wormmeta_lmost_load(meta) == wormmeta_rmost_load(meta)) { // delete single-child
      debug_assert(wormmeta_lmost_load(meta) == leaf);
      const u32 bitmin = wormmeta_bitmin_load(meta);
      wormhmap_del(hmap, meta);
      wormmeta_free(hmap, meta);
      if (parent) {
        wormmeta_bm_clear_helper(hmap, parent, pbuf->kv[i-1]);
        parent = NULL;
      }
      if (bitmin == WH_FO) // no child
        break;
    } else { // adjust lmost rmost
      if (wormmeta_lmost_load(meta) == leaf)
        wormmeta_lmost_store(meta, next);
      else if (wormmeta_rmost_load(meta) == leaf)
        wormmeta_rmost_store(meta, prev);
      parent = meta;
    }

    if (i >= alen)
      break;
    i++;
    wormhole_prefix_inc1(pbuf);
  } while (true);

  if (next)
    wormmeta_lpath_update(hmap, leaf->anchor, next->anchor, prev);
}

// all locks (metalock + two leaflock) will be released before returning
// merge leaf2 to leaf1, removing all metadata to leaf2 and leaf2 itself
  static void
wormhole_meta_merge(struct wormref_int * const ref, struct wormleaf_int * const leaf1,
    struct wormleaf_int * const leaf2, const bool unlock_leaf1)
{
  debug_assert(leaf1->next == leaf2);
  debug_assert(leaf2->prev == leaf1);
  struct wormhole_int * const map = ref->map;

  wormhmap_lock(map, ref);

  struct wormhmap * const hmap0 = wormhmap_load(map);
  struct wormhmap * const hmap1 = wormhmap_switch(map, hmap0);
  const u64 v1 = wormhmap_version_load(hmap0) + 1;

  leaf1->next = leaf2->next;
  if (leaf2->next)
    leaf2->next->prev = leaf1;

  wormleaf_int_version_store(leaf1, v1);
  wormleaf_int_version_store(leaf2, v1);
  wormhmap_version_store(hmap1, v1);

  wormmeta_merge(hmap1, leaf2);

  qsbr_update(&ref->qref, v1);

  // switch hmap
  wormhmap_store(map, hmap1);

  if (unlock_leaf1)
    wormleaf_int_unlock_write(leaf1);
  wormleaf_int_unlock_write(leaf2);

  qsbr_wait(map->qsbr, v1);

  wormmeta_merge(hmap0, leaf2);
  // leaf2 is now safe to be removed
  wormleaf_int_free(map->slab_leaf, leaf2);
  wormhmap_unlock(map);
}

// caller must acquire leaf->wlock and next->wlock
// all locks will be released when this function returns
  static bool
wormhole_meta_leaf_merge(struct wormref_int * const ref, struct wormleaf_int * const leaf)
{
  struct wormleaf_int * const next = leaf->next;
  debug_assert(next);

  // double check
  if ((leaf->nr_keys + next->nr_keys) <= WH_KPN) {
    if (wormleaf_merge(leaf, next)) {
      wormhole_meta_merge(ref, leaf, next, true);
      return true;
    }
  }
  // merge failed but it's fine
  wormleaf_int_unlock_write(leaf);
  wormleaf_int_unlock_write(next);
  return false;
}

  static void
whunsafe_int_meta_leaf_merge(struct wormhole_int * const map, struct wormleaf_int * const leaf1,
    struct wormleaf_int * const leaf2)
{
  debug_assert(leaf1->next == leaf2);
  debug_assert(leaf2->prev == leaf1);
  if (!wormleaf_merge(leaf1, leaf2))
    return;

  leaf1->next = leaf2->next;
  if (leaf2->next)
    leaf2->next->prev = leaf1;
  for (u32 i = 0; i < 2; i++)
    if (map->hmap2[i].pmap)
      wormmeta_merge(&(map->hmap2[i]), leaf2);
  wormleaf_int_free(map->slab_leaf, leaf2);
}
// }}} meta-merge

// put {{{
  bool
wormhole_int_put(struct wormref_int * const ref, struct kv * const kv)
{
  struct wormhole_int * const map = ref->map;
  if (unlikely(kv == NULL))
    return false;
  const struct kref kref = kv_kref(kv);

  struct wormleaf_int * const leaf = wormhole_jump_leaf_write(ref, &kref);
  // update
  const u32 im = wormleaf_int_search_eq(leaf, &kref);
  if (im < WH_KPN) {
    wormleaf_int_update(leaf, im, kv);
    wormleaf_int_unlock_write(leaf);
    return true;
  }

  // insert
  if (likely(leaf->nr_keys < WH_KPN)) { // just insert
    wormleaf_int_insert(leaf, kv);
    wormleaf_int_unlock_write(leaf);
    return true;
  }

  // split_insert changes hmap
  // all locks should be released in wormhole_split_insert()
  const bool rsi = wormhole_split_insert(ref, leaf, kv);
  return rsi;
}

  bool
whsafe_int_put(struct wormref_int * const ref, struct kv * const kv)
{
  wormhole_int_resume(ref);
  const bool r = wormhole_int_put(ref, kv);
  wormhole_int_park(ref);
  return r;
}

  bool
whunsafe_int_put(struct wormhole_int * const map, struct kv * const kv)
{
  if (unlikely(kv == NULL))
    return false;
  const struct kref kref = kv_kref(kv);

  struct wormleaf_int * const leaf = wormhole_jump_leaf(map->hmap, &kref);
  // update
  const u32 im = wormleaf_int_search_eq(leaf, &kref);
  if (im < WH_KPN) { // overwrite
    wormleaf_int_update(leaf, im, kv);
    return true;
  }

  // insert
  if (likely(leaf->nr_keys < WH_KPN)) { // just insert
    wormleaf_int_insert(leaf, kv);
    return true;
  }

  // split_insert changes hmap
  const bool rsi = whunsafe_split_insert(map, leaf, kv);
  return rsi;
}

  bool
wormhole_int_merge(struct wormref_int * const ref, const struct kref * const kref,
    kv_merge_func uf, void * const priv)
{
    return false;
}

  bool
whsafe_int_merge(struct wormref_int * const ref, const struct kref * const kref,
    kv_merge_func uf, void * const priv)
{
  wormhole_int_resume(ref);
  const bool r = wormhole_int_merge(ref, kref, uf, priv);
  wormhole_int_park(ref);
  return r;
}

  bool
whunsafe_int_merge(struct wormhole_int * const map, const struct kref * const kref,
    kv_merge_func uf, void * const priv)
{
    return false;
}
// }}} put

// del {{{
  static void
wormhole_del_try_merge(struct wormref_int * const ref, struct wormleaf_int * const leaf)
{
  struct wormleaf_int * const next = leaf->next;
  if (next && ((leaf->nr_keys == 0) || ((leaf->nr_keys + next->nr_keys) < WH_KPN_MRG))) {
    // try merge, it may fail if size becomes larger after locking
    wormleaf_int_lock_write(next, ref);
    (void)wormhole_meta_leaf_merge(ref, leaf);
    // locks are already released; immediately return
  } else {
    wormleaf_int_unlock_write(leaf);
  }
}

  bool
wormhole_int_del(struct wormref_int * const ref, const struct kref * const key)
{
  struct wormleaf_int * const leaf = wormhole_jump_leaf_write(ref, key);
  const u32 im = wormleaf_int_search_eq(leaf, key);
  if (im < WH_KPN) { // found
    wormleaf_int_remove(leaf, im);
    wormhole_del_try_merge(ref, leaf);
    debug_assert(kv);
    return true;
  } else {
    wormleaf_int_unlock_write(leaf);
    return false;
  }
}

  bool
whsafe_int_del(struct wormref_int * const ref, const struct kref * const key)
{
  wormhole_int_resume(ref);
  const bool r = wormhole_int_del(ref, key);
  wormhole_int_park(ref);
  return r;
}

  static void
whunsafe_int_del_try_merge(struct wormhole_int * const map, struct wormleaf_int * const leaf)
{
  const u32 n0 = leaf->prev ? leaf->prev->nr_keys : WH_KPN;
  const u32 n1 = leaf->nr_keys;
  const u32 n2 = leaf->next ? leaf->next->nr_keys : WH_KPN;

  if ((leaf->prev && (n1 == 0)) || ((n0 + n1) < WH_KPN_MRG)) {
    whunsafe_int_meta_leaf_merge(map, leaf->prev, leaf);
  } else if ((leaf->next && (n1 == 0)) || ((n1 + n2) < WH_KPN_MRG)) {
    whunsafe_int_meta_leaf_merge(map, leaf, leaf->next);
  }
}

  bool
whunsafe_int_del(struct wormhole_int * const map, const struct kref * const key)
{
  struct wormleaf_int * const leaf = wormhole_jump_leaf(map->hmap, key);
  const u32 im = wormleaf_int_search_eq(leaf, key);
  if (im < WH_KPN) { // found
    wormleaf_int_remove(leaf, im);
    debug_assert(kv);

    whunsafe_int_del_try_merge(map, leaf);
    return true;
  }
  return false;
}

  u64
wormhole_int_delr(struct wormref_int * const ref, const struct kref * const start,
    const struct kref * const end)
{
  struct wormleaf_int * const leafa = wormhole_jump_leaf_write(ref, start);
  wormleaf_int_sync_sorted(leafa);
  const u32 ia = wormleaf_int_seek(leafa, start);
  const u32 iaz = end ? wormleaf_int_seek_end(leafa, end) : leafa->nr_keys;
  if (iaz < ia) { // do nothing if end < start
    wormleaf_int_unlock_write(leafa);
    return 0;
  }
  u64 ndel = iaz - ia;
  struct wormhole_int * const map = ref->map;
  wormleaf_int_delete_range(leafa, ia, iaz);
  if (leafa->nr_keys > ia) { // end hit; done
    wormhole_del_try_merge(ref, leafa);
    return ndel;
  }

  while (leafa->next) {
    struct wormleaf_int * const leafx = leafa->next;
    wormleaf_int_lock_write(leafx, ref);
    // two leaf nodes locked
    wormleaf_int_sync_sorted(leafx);
    const u32 iz = end ? wormleaf_int_seek_end(leafx, end) : leafx->nr_keys;
    ndel += iz;
    wormleaf_int_delete_range(leafx, 0, iz);
    if (leafx->nr_keys == 0) { // removed all
      // must hold leaf1's lock for the next iteration
      wormhole_meta_merge(ref, leafa, leafx, false);
    } else { // partially removed; done
      (void)wormhole_meta_leaf_merge(ref, leafa);
      return ndel;
    }
  }
  wormleaf_int_unlock_write(leafa);
  return ndel;
}

  u64
whsafe_int_delr(struct wormref_int * const ref, const struct kref * const start,
    const struct kref * const end)
{
  wormhole_int_resume(ref);
  const u64 ret = wormhole_int_delr(ref, start, end);
  wormhole_int_park(ref);
  return ret;
}

  u64
whunsafe_int_delr(struct wormhole_int * const map, const struct kref * const start,
    const struct kref * const end)
{
  // first leaf
  struct wormhmap * const hmap = map->hmap;
  struct wormleaf_int * const leafa = wormhole_jump_leaf(hmap, start);
  wormleaf_int_sync_sorted(leafa);
  // last leaf
  struct wormleaf_int * const leafz = end ? wormhole_jump_leaf(hmap, end) : NULL;

  // select start/end on leafa
  const u32 ia = wormleaf_int_seek(leafa, start);
  const u32 iaz = end ? wormleaf_int_seek_end(leafa, end) : leafa->nr_keys;
  if (iaz < ia)
    return 0;

  wormleaf_int_delete_range(leafa, ia, iaz);
  u64 ndel = iaz - ia;

  if (leafa == leafz) { // one node only
    whunsafe_int_del_try_merge(map, leafa);
    return ndel;
  }

  // 0 or more nodes between leafa and leafz
  while (leafa->next != leafz) {
    struct wormleaf_int * const leafx = leafa->next;
    ndel += leafx->nr_keys;
    leafx->nr_keys = 0;
    leafx->nr_sorted = 0;
    whunsafe_int_meta_leaf_merge(map, leafa, leafx);
  }
  // delete the smaller keys in leafz
  if (leafz) {
    wormleaf_int_sync_sorted(leafz);
    const u32 iz = wormleaf_int_seek_end(leafz, end);
    wormleaf_int_delete_range(leafz, 0, iz);
    ndel += iz;
    whunsafe_int_del_try_merge(map, leafa);
  }
  return ndel;
}
// }}} del

// iter {{{
// safe iter: safe sort with read-lock acquired
// unsafe iter: allow concurrent seek/skip
  static void
wormhole_int_iter_leaf_sync_sorted(struct wormleaf_int * const leaf)
{
  if (unlikely(leaf->nr_keys != leaf->nr_sorted)) {
    spinlock_lock(&(leaf->sortlock));
    wormleaf_int_sync_sorted(leaf);
    spinlock_unlock(&(leaf->sortlock));
  }
}

  struct wormhole_int_iter *
wormhole_int_iter_create(struct wormref_int * const ref)
{
  struct wormhole_int_iter * const iter = malloc(sizeof(*iter));
  if (iter == NULL)
    return NULL;
  iter->ref = ref;
  iter->map = ref->map;
  iter->leaf = NULL;
  iter->is = 0;
  return iter;
}

  static void
wormhole_int_iter_fix(struct wormhole_int_iter * const iter)
{
  if (!wormhole_int_iter_valid(iter))
    return;

  while (unlikely(iter->is >= iter->leaf->nr_sorted)) {
    struct wormleaf_int * const next = iter->leaf->next;
    if (likely(next != NULL)) {
      struct wormref_int * const ref = iter->ref;
      wormleaf_int_lock_read(next, ref);
      wormleaf_int_unlock_read(iter->leaf);

      wormhole_int_iter_leaf_sync_sorted(next);
    } else {
      wormleaf_int_unlock_read(iter->leaf);
    }
    iter->leaf = next;
    iter->is = 0;
    if (!wormhole_int_iter_valid(iter))
      return;
  }
}

  static void
wormhole_int_iter_fix_rev(struct wormhole_int_iter * const iter)
{
  if (!wormhole_int_iter_valid(iter))
    return;

  while (unlikely(iter->is < 0)) {
    struct wormleaf_int * const prev = iter->leaf->prev;
    if (likely(prev != NULL)) {
      struct wormref_int * const ref = iter->ref;
      wormleaf_int_lock_read(prev, ref);
      wormleaf_int_unlock_read(iter->leaf);

      wormhole_int_iter_leaf_sync_sorted(prev);
    } else {
      wormleaf_int_unlock_read(iter->leaf);
    }
    iter->leaf = prev;
    if (!wormhole_int_iter_valid(iter))
      return;
    iter->is = prev->nr_keys - 1;
  }
}

  void
wormhole_int_iter_seek(struct wormhole_int_iter * const iter, const struct kref * const key)
{
  debug_assert(key);
  if (iter->leaf)
    wormleaf_int_unlock_read(iter->leaf);

  struct wormleaf_int * const leaf = wormhole_jump_leaf_read(iter->ref, key);
  wormhole_int_iter_leaf_sync_sorted(leaf);

  iter->leaf = leaf;
  iter->is = wormleaf_int_seek(leaf, key);
  wormhole_int_iter_fix(iter);
}

  void
whsafe_int_iter_seek(struct wormhole_int_iter * const iter, const struct kref * const key)
{
  wormhole_int_resume(iter->ref);
  wormhole_int_iter_seek(iter, key);
}

  bool
wormhole_int_iter_valid(struct wormhole_int_iter * const iter)
{
  return iter->leaf != NULL;
}

  static struct int_store_pair *
wormhole_int_iter_current(struct wormhole_int_iter * const iter)
{
  if (wormhole_int_iter_valid(iter)) {
    debug_assert(iter->is < iter->leaf->nr_sorted);
    return iter->leaf->kvs + iter->is;
  }
  return NULL;
}

  struct kv *
wormhole_int_iter_peek(struct wormhole_int_iter * const iter, struct kv * const out)
{
  struct int_store_pair * const res = wormhole_int_iter_current(iter);
  if (res) {
    out->klen = res->key_size;
    memcpy(out->kv, &res->key, out->klen);
    out->vlen = sizeof(res->store);
    memcpy(out->kv + out->klen, res->store, out->vlen);
    return out;
  }
  return NULL;
}

  bool
wormhole_int_iter_kref(struct wormhole_int_iter * const iter, struct kref * const kref)
{
  return false;
}


  bool
wormhole_int_iter_kvref(struct wormhole_int_iter * const iter, struct kvref * const kvref)
{
  return false;
}

  void
wormhole_int_iter_skip1(struct wormhole_int_iter * const iter)
{
  if (wormhole_int_iter_valid(iter)) {
    iter->is++;
    wormhole_int_iter_fix(iter);
  }
}

  void
wormhole_int_iter_skip(struct wormhole_int_iter * const iter, const u32 nr)
{
  u32 todo = nr;
  while (todo && wormhole_int_iter_valid(iter)) {
    const u32 cap = iter->leaf->nr_sorted - iter->is;
    const u32 nskip = (cap < todo) ? cap : todo;
    iter->is += nskip;
    wormhole_int_iter_fix(iter);
    todo -= nskip;
  }
}

  struct kv *
wormhole_int_iter_next(struct wormhole_int_iter * const iter, struct kv * const out)
{
  struct kv * const ret = wormhole_int_iter_peek(iter, out);
  wormhole_int_iter_skip1(iter);
  return ret;
}

  void
wormhole_int_iter_skip1_rev(struct wormhole_int_iter * const iter)
{
  if (wormhole_int_iter_valid(iter)) {
    iter->is--;
    wormhole_int_iter_fix_rev(iter);
  }
}


  struct kv *
wormhole_int_iter_prev(struct wormhole_int_iter * const iter, struct kv * const out)
{
  struct kv * const ret = wormhole_int_iter_peek(iter, out);
  wormhole_int_iter_skip1_rev(iter);
  return ret;
}

  bool
wormhole_int_iter_inp(struct wormhole_int_iter * const iter, kv_inp_func uf, void * const priv)
{
  struct int_store_pair * const kv = wormhole_int_iter_current(iter);
  uint8_t buf[sizeof(struct kv) + kv->key_size + sizeof(kv->store)];
  struct kv * res = (struct kv *) buf;
  res->klen = kv->key_size;
  memcpy(res->kv, &kv->key, res->klen);
  res->vlen = sizeof(kv->store);
  memcpy(res->kv + kv->key_size, kv->store, res->vlen);

  uf(res, priv); // call uf even if (kv == NULL)
  return kv != NULL;
}

  void
wormhole_int_iter_park(struct wormhole_int_iter * const iter)
{
  if (iter->leaf) {
    wormleaf_int_unlock_read(iter->leaf);
    iter->leaf = NULL;
  }
}

  void
whsafe_int_iter_park(struct wormhole_int_iter * const iter)
{
  wormhole_int_iter_park(iter);
  wormhole_int_park(iter->ref);
}

  void
wormhole_int_iter_destroy(struct wormhole_int_iter * const iter)
{
  if (iter->leaf)
    wormleaf_int_unlock_read(iter->leaf);
  free(iter);
}

  void
whsafe_int_iter_destroy(struct wormhole_int_iter * const iter)
{
  wormhole_int_park(iter->ref);
  wormhole_int_iter_destroy(iter);
}
// }}} iter

// misc {{{
  struct wormref_int *
wormhole_int_ref(struct wormhole_int * const map)
{
  struct wormref_int * const ref = malloc(sizeof(*ref));
  if (ref == NULL)
    return NULL;
  ref->map = map;
  if (qsbr_register(map->qsbr, &(ref->qref)) == false) {
    free(ref);
    return NULL;
  }
  return ref;
}

  struct wormref_int *
whsafe_int_ref(struct wormhole_int * const map)
{
  struct wormref_int * const ref = wormhole_int_ref(map);
  if (ref)
    wormhole_int_park(ref);
  return ref;
}

  struct wormhole_int *
wormhole_int_unref(struct wormref_int * const ref)
{
  struct wormhole_int * const map = ref->map;
  qsbr_unregister(map->qsbr, &(ref->qref));
  free(ref);
  return map;
}

  inline void
wormhole_int_park(struct wormref_int * const ref)
{
  qsbr_park(&(ref->qref));
}

  inline void
wormhole_int_resume(struct wormref_int * const ref)
{
  qsbr_resume(&(ref->qref));
}

  inline void
wormhole_int_refresh_qstate(struct wormref_int * const ref)
{
  qsbr_update(&(ref->qref), wormhmap_version_load(wormhmap_load(ref->map)));
}

  static void
wormhole_clean_hmap(struct wormhole_int * const map)
{
  for (u32 x = 0; x < 2; x++) {
    if (map->hmap2[x].pmap == NULL)
      continue;
    struct wormhmap * const hmap = &(map->hmap2[x]);
    const u64 nr_slots = ((u64)(hmap->mask)) + 1;
    struct wormmbkt * const pmap = hmap->pmap;
    for (u64 s = 0; s < nr_slots; s++) {
      struct wormmbkt * const slot = &(pmap[s]);
      for (u32 i = 0; i < WH_BKT_NR; i++)
        if (slot->e[i])
          wormmeta_keyref_release(slot->e[i]);
    }

    slab_free_all(hmap->slab1);
    slab_free_all(hmap->slab2);
    memset(hmap->pmap, 0, hmap->msize);
    hmap->maxplen = 0;
  }
}

  static void
wormhole_free_leaf_keys(struct wormhole_int * const map, struct wormleaf_int * const leaf)
{
  wormhole_free_akey(leaf->anchor);
}

  static void
wormhole_clean_helper(struct wormhole_int * const map)
{
  wormhole_clean_hmap(map);
  for (struct wormleaf_int * leaf = map->leaf0; leaf; leaf = leaf->next)
    wormhole_free_leaf_keys(map, leaf);
  slab_free_all(map->slab_leaf);
  map->leaf0 = NULL;
}

// unsafe
  void
wormhole_int_clean(struct wormhole_int * const map)
{
  wormhole_clean_helper(map);
  wormhole_create_leaf0(map);
}

  void
wormhole_int_destroy(struct wormhole_int * const map)
{
  wormhole_clean_helper(map);
  for (u32 i = 0; i < 2; i++) {
    struct wormhmap * const hmap = &map->hmap2[i];
    if (hmap->slab1)
      slab_destroy(hmap->slab1);
    if (hmap->slab2)
      slab_destroy(hmap->slab2);
    wormhmap_deinit(hmap);
  }
  qsbr_destroy(map->qsbr);
  slab_destroy(map->slab_leaf);
  free(map->pbuf);
  free(map);
}

  void
wormhole_int_fprint(struct wormhole_int * const map, FILE * const out)
{
  const u64 nr_slab_ul = slab_get_nalloc(map->slab_leaf);
  const u64 nr_slab_um11 = slab_get_nalloc(map->hmap2[0].slab1);
  const u64 nr_slab_um12 = slab_get_nalloc(map->hmap2[0].slab2);
  const u64 nr_slab_um21 = map->hmap2[1].slab1 ? slab_get_nalloc(map->hmap2[1].slab1) : 0;
  const u64 nr_slab_um22 = map->hmap2[1].slab2 ? slab_get_nalloc(map->hmap2[1].slab2) : 0;
  fprintf(out, "%s L-SLAB %lu M-SLAB [0] %lu+%lu [1] %lu+%lu\n",
      __func__, nr_slab_ul, nr_slab_um11, nr_slab_um12, nr_slab_um21, nr_slab_um22);
}
// }}} misc

// api {{{
const struct kvmap_api kvmap_api_wormhole_int = {
  .hashkey = true,
  .ordered = true,
  .threadsafe = true,
  .unique = true,
  .refpark = true,
  .put = (void *)wormhole_int_put,
  .get = (void *)wormhole_int_get,
  .probe = (void *)wormhole_int_probe,
  .del = (void *)wormhole_int_del,
  .inpr = NULL,
  .inpw = NULL,
  .merge = (void *)wormhole_int_merge,
  .delr = (void *)wormhole_int_delr,
  .iter_create = (void *)wormhole_int_iter_create,
  .iter_seek = (void *)wormhole_int_iter_seek,
  .iter_valid = (void *)wormhole_int_iter_valid,
  .iter_peek = (void *)wormhole_int_iter_peek,
  .iter_kref = (void *)wormhole_int_iter_kref,
  .iter_kvref = (void *)wormhole_int_iter_kvref,
  .iter_skip1 = (void *)wormhole_int_iter_skip1,
  .iter_skip = (void *)wormhole_int_iter_skip,
  .iter_skip1_rev = (void *)wormhole_int_iter_skip1_rev,
  .iter_next = (void *)wormhole_int_iter_next,
  .iter_inp = (void *)wormhole_int_iter_inp,
  .iter_park = (void *)wormhole_int_iter_park,
  .iter_destroy = (void *)wormhole_int_iter_destroy,
  .ref = (void *)wormhole_int_ref,
  .unref = (void *)wormhole_int_unref,
  .park = (void *)wormhole_int_park,
  .resume = (void *)wormhole_int_resume,
  .clean = (void *)wormhole_int_clean,
  .destroy = (void *)wormhole_int_destroy,
  .fprint = (void *)wormhole_int_fprint,
};

const struct kvmap_api kvmap_api_whsafe_int = {
  .hashkey = true,
  .ordered = true,
  .threadsafe = true,
  .unique = true,
  .put = (void *)whsafe_int_put,
  .get = (void *)whsafe_int_get,
  .probe = (void *)whsafe_int_probe,
  .del = (void *)whsafe_int_del,
  .inpr = NULL,
  .inpw = NULL,
  .merge = (void *)whsafe_int_merge,
  .delr = (void *)whsafe_int_delr,
  .iter_create = (void *)wormhole_int_iter_create,
  .iter_seek = (void *)whsafe_int_iter_seek,
  .iter_valid = (void *)wormhole_int_iter_valid,
  .iter_peek = (void *)wormhole_int_iter_peek,
  .iter_kref = (void *)wormhole_int_iter_kref,
  .iter_kvref = (void *)wormhole_int_iter_kvref,
  .iter_skip1 = (void *)wormhole_int_iter_skip1,
  .iter_skip = (void *)wormhole_int_iter_skip,
  .iter_skip1_rev = (void *)wormhole_int_iter_skip1_rev,
  .iter_next = (void *)wormhole_int_iter_next,
  .iter_inp = (void *)wormhole_int_iter_inp,
  .iter_park = (void *)whsafe_int_iter_park,
  .iter_destroy = (void *)whsafe_int_iter_destroy,
  .ref = (void *)whsafe_int_ref,
  .unref = (void *)wormhole_int_unref,
  .clean = (void *)wormhole_int_clean,
  .destroy = (void *)wormhole_int_destroy,
  .fprint = (void *)wormhole_int_fprint,
};

  static void *
wormhole_kvmap_api_create(const char * const name, const struct kvmap_mm * const mm, char ** args)
{
  (void)args;
  if ((!strcmp(name, "wormhole")) || (!strcmp(name, "whsafe"))) {
    return wormhole_int_create(mm);
  } else {
    return NULL;
  }
}

__attribute__((constructor))
  static void
wormhole_kvmap_api_init(void)
{
  kvmap_api_register(0, "wormhole", "", wormhole_kvmap_api_create, &kvmap_api_wormhole_int);
  kvmap_api_register(0, "whsafe", "", wormhole_kvmap_api_create, &kvmap_api_whsafe_int);
}
// }}} api

// wh {{{
// Users often don't enjoy dealing with struct kv/kref and just want to use plain buffers.
// No problem!
// This example library shows you how to use Wormhole efficiently in the most intuitive way.

// Use the worry-free api
static const struct kvmap_api * const wh_api = &kvmap_api_whsafe_int;

// You can change the wh_api to kvmap_api_wormhole with a one-line replacement
// The standard Wormhole api can give you ~5% boost; read README for thread-safety tips
//static const struct kvmap_api * const wh_api = &kvmap_api_wormhole;

  struct wormhole_int *
wh_int_create(void)
{
  // kvmap_mm_ndf (kv.h) will let the caller allocate the kv when inserting
  // This can avoid a memcpy if the caller does not have the data in a struct kv
  return wormhole_int_create(&kvmap_mm_ndf);
}

  struct wormref_int *
wh_int_ref(struct wormhole_int * const wh)
{
  return wh_api->ref(wh);
}

  void
wh_int_unref(struct wormref_int * const ref)
{
  (void)wh_api->unref(ref);
}

  void
wh_int_park(struct wormref_int * const ref)
{
  if (wh_api->park)
    wh_api->park(ref);
}

  void
wh_int_resume(struct wormref_int * const ref)
{
  if (wh_api->resume)
    wh_api->resume(ref);
}

  void
wh_int_clean(struct wormhole_int * const map)
{
  wh_api->clean(map);
}

  void
wh_int_destroy(struct wormhole_int * const map)
{
  wh_api->destroy(map);
}

// Do set/put with explicit kv buffers
  bool
wh_int_put(struct wormref_int * const ref, const void * const kbuf, const u32 klen,
    const void * const vbuf, const u32 vlen)
{
  struct kv * const newkv = kv_create(kbuf, klen, vbuf, vlen);

  if (newkv == NULL) {
      free(newkv);
    return false;
  }
  // must use with kvmap_mm_ndf (see below)
  // the newkv will be saved in the Wormhole and freed by Wormhole when upon deletion
  bool res = wh_api->put(ref, newkv);
  free(newkv);
  return res;
}

// delete a key
  bool
wh_int_del(struct wormref_int * const ref, const void * const kbuf, const u32 klen)
{
  struct kref kref;
  kref_ref_hash32(&kref, kbuf, klen);
  return wh_api->del(ref, &kref);
}

// test if the key exist in Wormhole
  bool
wh_int_probe(struct wormref_int * const ref, const void * const kbuf, const u32 klen)
{
  struct kref kref;
  kref_ref_hash32(&kref, kbuf, klen);
  return wh_api->probe(ref, &kref);
}

// for wh_get()
struct wh_inp_info { void * vbuf_out; u32 * vlen_out; u32 vbuf_size; };

// a kv_inp_func; use this to retrieve the KV's data without unnecesary memory copying
  static void
wh_inp_copy_value(struct kv * const curr, void * const priv)
{
  if (curr) { // found
    struct wh_inp_info * const info = (typeof(info))priv;
    // copy the value data out
    const u32 copy_size = info->vbuf_size < curr->vlen ? info->vbuf_size : curr->vlen;
    memcpy(info->vbuf_out, kv_vptr_c(curr), copy_size);
    // copy the vlen out
    *info->vlen_out = curr->vlen;
  }
}

// returns a boolean value indicating whether the key is found.
// the value's data will be written to *vlen_out and vbuf_out if the key is found
// if vbuf_size < vlen, then only the first vbuf_size bytes is copied to the buffer
// a small vbuf_size can be used to reduce memcpy cost when only the first a few bytes are needed
  bool
wh_int_get(struct wormref_int * const ref, const void * const kbuf, const u32 klen,
    void * const vbuf_out, const u32 vbuf_size, u32 * const vlen_out)
{
  struct kref kref;
  kref_ref_hash32(&kref, kbuf, klen);
  struct wh_inp_info info = {vbuf_out, vlen_out, vbuf_size};
  // use the inplace read function to get the value if it exists
  return wh_api->inpr(ref, &kref, wh_inp_copy_value, &info);
}

  bool
wh_int_inpr(struct wormref_int * const ref, const void * const kbuf, const u32 klen,
    kv_inp_func uf, void * const priv)
{
  struct kref kref;
  kref_ref_hash32(&kref, kbuf, klen);
  return wh_api->inpr(ref, &kref, uf, priv);
}

// inplace update KV's value with a user-defined hook function
// the update should only modify the data in the value; It should not change the value size
  bool
wh_int_inpw(struct wormref_int * const ref, const void * const kbuf, const u32 klen,
    kv_inp_func uf, void * const priv)
{
  struct kref kref;
  kref_ref_hash32(&kref, kbuf, klen);
  return wh_api->inpw(ref, &kref, uf, priv);
}

// merge existing KV with updates with a user-defined hook function
  bool
wh_int_merge(struct wormref_int * const ref, const void * const kbuf, const u32 klen,
    kv_merge_func uf, void * const priv)
{
  struct kref kref;
  kref_ref_hash32(&kref, kbuf, klen);
  return wh_api->merge(ref, &kref, uf, priv);
}

// remove a range of KVs from start (inclusive) to end (exclusive); [start, end)
  u64
wh_int_delr(struct wormref_int * const ref, const void * const kbuf_start, const u32 klen_start,
    const void * const kbuf_end, const u32 klen_end)
{
  struct kref kref_start, kref_end;
  kref_ref_hash32(&kref_start, kbuf_start, klen_start);
  kref_ref_hash32(&kref_end, kbuf_end, klen_end);
  return wh_api->delr(ref, &kref_start, &kref_end);
}

  struct wormhole_int_iter *
wh_int_iter_create(struct wormref_int * const ref)
{
  return wh_api->iter_create(ref);
}

  void
wh_int_iter_seek(struct wormhole_int_iter * const iter, const void * const kbuf, const u32 klen)
{
  struct kref kref;
  kref_ref_hash32(&kref, kbuf, klen);
  wh_api->iter_seek(iter, &kref);
}

  bool
wh_int_iter_valid(struct wormhole_int_iter * const iter)
{
  return wh_api->iter_valid(iter);
}

// for wh_int_iter_peek()
// the out ptrs must be provided in pairs; use a pair of NULLs to ignore the key or value
struct wh_int_iter_inp_info { void * kbuf_out; void * vbuf_out; u32 kbuf_size; u32 vbuf_size; u32 * klen_out; u32 * vlen_out; };

// a kv_inp_func; use this to retrieve the KV's data without unnecesary memory copying
  static void
inp_copy_kv_cb(struct kv * const curr, void * const priv)
{
  if (curr) { // found
    struct wh_int_iter_inp_info * const info = (typeof(info))priv;

    // copy the key
    if (info->kbuf_out) { // it assumes klen_out is also not NULL
      // copy the key data out
      const u32 clen = curr->klen < info->kbuf_size ? curr->klen : info->kbuf_size;
      memcpy(info->kbuf_out, kv_kptr_c(curr), clen);
      // copy the klen out
      *info->klen_out = curr->klen;
    }

    // copy the value
    if (info->vbuf_out) { // it assumes vlen_out is also not NULL
      // copy the value data out
      const u32 clen = curr->vlen < info->vbuf_size ? curr->vlen : info->vbuf_size;
      memcpy(info->vbuf_out, kv_vptr_c(curr), clen);
      // copy the vlen out
      *info->vlen_out = curr->vlen;
    }
  }
}

// seek is similar to get
  bool
wh_int_iter_peek(struct wormhole_int_iter * const iter,
    void * const kbuf_out, const u32 kbuf_size, u32 * const klen_out,
    void * const vbuf_out, const u32 vbuf_size, u32 * const vlen_out)
{
  struct wh_int_iter_inp_info info = {kbuf_out, vbuf_out, kbuf_size, vbuf_size, klen_out, vlen_out};
  return wh_api->iter_inp(iter, inp_copy_kv_cb, &info);
}

// seek is similar to get
  void
wh_int_iter_peek_ref(struct wormhole_int_iter * const iter,
    const void ** kbuf_out, u32 * const klen_out,
    void ** vbuf_out, u32 * const vlen_out)
{
    *kbuf_out = &(iter->leaf->kvs[iter->is].key);
    *klen_out = iter->leaf->kvs[iter->is].key_size;
    *vbuf_out = &(iter->leaf->kvs[iter->is].store);
    *vlen_out = sizeof(iter->leaf->kvs[iter->is].store);
}

  void
wh_int_iter_skip1(struct wormhole_int_iter * const iter)
{
  wh_api->iter_skip1(iter);
}

  void
wh_int_iter_skip1_rev(struct wormhole_int_iter * const iter)
{
  wh_api->iter_skip1_rev(iter);
}

  void
wh_int_iter_skip(struct wormhole_int_iter * const iter, const u32 nr)
{
  wh_api->iter_skip(iter, nr);
}

  bool
wh_int_iter_inp(struct wormhole_int_iter * const iter, kv_inp_func uf, void * const priv)
{
  return wh_api->iter_inp(iter, uf, priv);
}

  void
wh_int_iter_park(struct wormhole_int_iter * const iter)
{
  wh_api->iter_park(iter);
}

  void
wh_int_iter_destroy(struct wormhole_int_iter * const iter)
{
  wh_api->iter_destroy(iter);
}
// }}} wh

#undef BITMASK

// vim:fdm=marker
