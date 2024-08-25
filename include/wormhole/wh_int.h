/*
 * Copyright (c) 2016--2021  Wu, Xingbo <wuxb45@gmail.com>
 *
 * All rights reserved. No warranty, explicit or implicit, provided.
 */
#pragma once

#include "kv.h"

#ifdef __cplusplus
extern "C" {
#endif

struct wormhole_int;
struct wormref_int;

struct wormref_int {
  struct wormhole_int * map;
  struct qsbr_ref qref;
};

struct wormhole_int_iter {
  struct wormref_int * ref; // safe-iter only
  struct wormhole_int * map;
  struct wormleaf_int * leaf;
  int is;
};


// wormhole_int {{{
// the wh created by wormhole_create() can work with all of safe/unsafe operations.
  extern struct wormhole_int *
wormhole_int_create(const struct kvmap_mm * const mm);

// the wh created by whunsafe_create() can only work with the unsafe operations.
  extern struct wormhole_int *
whunsafe_int_create(const struct kvmap_mm * const mm);

  extern struct kv *
wormhole_int_get(struct wormref_int * const ref, const struct kref * const key, struct kv * const out);

  extern bool
wormhole_int_probe(struct wormref_int * const ref, const struct kref * const key);

  extern bool
wormhole_int_put(struct wormref_int * const ref, struct kv * const kv);

  extern bool
wormhole_int_merge(struct wormref_int * const ref, const struct kref * const kref,
    kv_merge_func uf, void * const priv);

  extern bool
wormhole_int_del(struct wormref_int * const ref, const struct kref * const key);

  extern u64
wormhole_int_delr(struct wormref_int * const ref, const struct kref * const start,
    const struct kref * const end);

// HAAAAAAAAAAAAAAACK
  void
wormleaf_int_unlock_read(struct wormleaf_int * const leaf);

  extern struct wormhole_int_iter *
wormhole_int_iter_create(struct wormref_int * const ref);

  extern void
wormhole_int_iter_seek(struct wormhole_int_iter * const iter, const struct kref * const key);

  extern bool
wormhole_int_iter_valid(struct wormhole_int_iter * const iter);

  extern struct kv *
wormhole_int_iter_peek(struct wormhole_int_iter * const iter, struct kv * const out);

  extern bool
wormhole_int_iter_kref(struct wormhole_int_iter * const iter, struct kref * const kref);

  extern bool
wormhole_int_iter_kvref(struct wormhole_int_iter * const iter, struct kvref * const kvref);

  extern void
wormhole_int_iter_skip1(struct wormhole_int_iter * const iter);

  extern void
wormhole_int_iter_skip(struct wormhole_int_iter * const iter, const u32 nr);

  extern struct kv *
wormhole_int_iter_next(struct wormhole_int_iter * const iter, struct kv * const out);

  extern void
wormhole_int_iter_skip1_rev(struct wormhole_int_iter * const iter);

  extern struct kv *
wormhole_int_iter_prev(struct wormhole_int_iter * const iter, struct kv * const out);


  extern bool
wormhole_int_iter_inp(struct wormhole_int_iter * const iter, kv_inp_func uf, void * const priv);

  extern void
wormhole_int_iter_park(struct wormhole_int_iter * const iter);

  extern void
wormhole_int_iter_destroy(struct wormhole_int_iter * const iter);

  extern struct wormref_int *
wormhole_int_ref(struct wormhole_int * const map);

  extern struct wormhole_int *
wormhole_int_unref(struct wormref_int * const ref);

  extern void
wormhole_int_park(struct wormref_int * const ref);

  extern void
wormhole_int_resume(struct wormref_int * const ref);

  extern void
wormhole_int_refresh_qstate(struct wormref_int * const ref);

// clean with more threads
  extern void
wormhole_int_clean_th(struct wormhole_int * const map, const u32 nr_threads);

  extern void
wormhole_int_clean(struct wormhole_int * const map);

  extern void
wormhole_int_destroy(struct wormhole_int * const map);

// safe API (no need to refresh qstate)

  extern struct kv *
whsafe_int_get(struct wormref_int * const ref, const struct kref * const key, struct kv * const out);

  extern bool
whsafe_int_probe(struct wormref_int * const ref, const struct kref * const key);

  extern bool
whsafe_int_put(struct wormref_int * const ref, struct kv * const kv);

  extern bool
whsafe_int_merge(struct wormref_int * const ref, const struct kref * const kref,
    kv_merge_func uf, void * const priv);

  extern bool
whsafe_int_del(struct wormref_int * const ref, const struct kref * const key);

  extern u64
whsafe_int_delr(struct wormref_int * const ref, const struct kref * const start,
    const struct kref * const end);

// use wormhole_int_iter_create
  extern void
whsafe_int_iter_seek(struct wormhole_int_iter * const iter, const struct kref * const key);

  extern struct kv *
whsafe_int_iter_peek(struct wormhole_int_iter * const iter, struct kv * const out);

// use wormhole_int_iter_valid
// use wormhole_int_iter_peek
// use wormhole_int_iter_kref
// use wormhole_int_iter_kvref
// use wormhole_int_iter_skip1
// use wormhole_int_iter_skip
// use wormhole_int_iter_next

  extern void
whsafe_int_iter_park(struct wormhole_int_iter * const iter);

  extern void
whsafe_int_iter_destroy(struct wormhole_int_iter * const iter);

  extern struct wormref_int *
whsafe_int_ref(struct wormhole_int * const map);

// use wormhole_unref

  extern void
wormhole_int_fprint(struct wormhole_int * const map, FILE * const out);

extern const struct kvmap_api kvmap_api_wormhole;
extern const struct kvmap_api kvmap_api_whsafe;
extern const struct kvmap_api kvmap_api_whunsafe;
// }}} wormhole_int

// wh {{{
  extern struct wormhole_int *
wh_int_create(void);

  extern struct wormref_int *
wh_int_ref(struct wormhole_int * const wh);

  extern void
wh_int_unref(struct wormref_int * const ref);

  extern void
wh_int_park(struct wormref_int * const ref);

  extern void
wh_int_resume(struct wormref_int * const ref);

  extern void
wh_int_clean(struct wormhole_int * const map);

  extern void
wh_int_destroy(struct wormhole_int * const map);

  extern bool
wh_int_put(struct wormref_int * const ref, const void * const kbuf, const u32 klen,
    const void * const vbuf, const u32 vlen);

  extern bool
wh_int_del(struct wormref_int * const ref, const void * const kbuf, const u32 klen);

  extern bool
wh_int_probe(struct wormref_int * const ref, const void * const kbuf, const u32 klen);

  extern bool
wh_int_get(struct wormref_int * const ref, const void * const kbuf, const u32 klen,
    void * const vbuf_out, const u32 vbuf_size, u32 * const vlen_out);

  extern bool
wh_int_merge(struct wormref_int * const ref, const void * const kbuf, const u32 klen,
    kv_merge_func uf, void * const priv);

  extern u64
wh_int_delr(struct wormref_int * const ref, const void * const kbuf_start, const u32 klen_start,
    const void * const kbuf_end, const u32 klen_end);

  extern struct wormhole_int_iter *
wh_int_iter_create(struct wormref_int * const ref);

  extern void
wh_int_iter_seek(struct wormhole_int_iter * const iter, const void * const kbuf, const u32 klen);

  extern bool
wh_int_iter_valid(struct wormhole_int_iter * const iter);

  extern bool
wh_int_iter_peek(struct wormhole_int_iter * const iter,
    void * const kbuf_out, const u32 kbuf_size, u32 * const klen_out,
    void * const vbuf_out, const u32 vbuf_size, u32 * const vlen_out);

  void
wh_int_iter_peek_ref(struct wormhole_int_iter * const iter,
    const void ** kbuf_out, u32 * const klen_out,
    void ** vbuf_out, u32 * const vlen_out);

  extern void
wh_int_iter_skip1(struct wormhole_int_iter * const iter);

  extern void
wh_int_iter_skip(struct wormhole_int_iter * const iter, const u32 nr);

  extern void
wh_int_iter_skip1_rev(struct wormhole_int_iter * const iter);

  extern bool
wh_int_iter_inp(struct wormhole_int_iter * const iter, kv_inp_func uf, void * const priv);

  extern void
wh_int_iter_park(struct wormhole_int_iter * const iter);

  extern void
wh_int_iter_destroy(struct wormhole_int_iter * const iter);
// }}} wh

#ifdef __cplusplus
}
#endif
// vim:fdm=marker

