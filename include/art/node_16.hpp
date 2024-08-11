/**
 * @file node_16 header
 * @author Rafael Kallis <rk@rafaelkallis.com>
 */

#ifndef ART_NODE_16_HPP
#define ART_NODE_16_HPP

#include "inner_node.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <sys/types.h>
#include <utility>

#if defined(__i386__) || defined(__amd64__)
#include <emmintrin.h>
#endif

namespace art {

template <class T> class node_4;
template <class T> class node_4_valued;
template <class T> class node_48;
template <class T> class node_48_valued;

template <class T> class node_16_valued;

template <class T> class node_16 : public inner_node<T> {
    friend class node_4<T>;
    friend class node_48<T>;

    friend class node_4_valued<T>;
    friend class node_16_valued<T>;
    friend class node_48_valued<T>;
public:
    node<T> **find_child(uint8_t partial_key) override;
    void set_child(uint8_t partial_key, node<T> *child) override;
    node<T> *del_child(uint8_t partial_key) override;
    inner_node<T> *grow() override;
    inner_node<T> *shrink() override;
    bool is_full() const override;
    bool is_underfull() const override;

    uint8_t next_partial_key(uint8_t partial_key) const override;

    uint8_t prev_partial_key(uint8_t partial_key) const override;

    int n_children() const override;

    inner_node<T> *set_value(T &value) override {
        node_16_valued<T> *res = new node_16_valued<T>();
        res->prefix_ = this->prefix_;
        res->prefix_len_ = this->prefix_len_;
        res->n_children_ = this->n_children_;
        memcpy(res->keys_, this->keys_, sizeof(res->keys_));
        memcpy(res->children_, this->children_, sizeof(res->children_));
        res->value_ = value;
        delete this;
        return res;
    };

    inner_node<T> *remove_value() override {
        throw std::runtime_error("cannot remove value from a node without a value");
    };

private:
    uint8_t n_children_ = 0;
    uint8_t keys_[16];
    node<T> *children_[16];
};


template <class T> class node_16_valued : public node_16<T> {
public:
    T *get_value() override {
        return reinterpret_cast<T *>(&value_);
    };

    inner_node<T> *grow() override {
        node_48_valued<T> *new_node = new node_48_valued<T>();
        new_node->prefix_ = this->prefix_;
        new_node->prefix_len_ = this->prefix_len_;
        new_node->n_children_ = this->n_children_;
        std::copy(this->children_, this->children_ + this->n_children_, new_node->children_);
        for (int i = 0; i < this->n_children_; ++i) {
            new_node->indexes_[(uint8_t) this->keys_[i]] = i;
        }
        new_node->value_ = this->value_;
        delete this;
        return new_node;
    }

    inner_node<T> *shrink() override {
        node_4_valued<T> *new_node = new node_4_valued<T>();
        new_node->prefix_ = this->prefix_;
        new_node->prefix_len_ = this->prefix_len_;
        new_node->n_children_ = this->n_children_;
        std::copy(this->keys_, this->keys_ + this->n_children_, new_node->keys_);
        std::copy(this->children_, this->children_ + this->n_children_, new_node->children_);
        new_node->value_ = this->value_;
        delete this;
        return new_node;
    }

    inner_node<T> *set_value(T &value) override {
        this->value_ = value;
        return this;
    };

    inner_node<T> *remove_value() override {
        node_16<T> *res = new node_16<T>();
        res->prefix_ = this->prefix_;
        res->prefix_len_ = this->prefix_len_;
        res->n_children_ = this->n_children_;
        memcpy(res->keys_, this->keys_, sizeof(res->keys_));
        memcpy(res->children_, this->children_, sizeof(res->children_));
        delete this;
        return res;
    }

    T value_;
};


template <class T> node<T> **node_16<T>::find_child(uint8_t partial_key) {
#if defined(__i386__) || defined(__amd64__)
    int bitfield =
        _mm_movemask_epi8(_mm_cmpeq_epi8(_mm_set1_epi8(partial_key),
                    _mm_loadu_si128((__m128i *)keys_))) &
        ((1 << n_children_) - 1);
    return (bool)bitfield ? &children_[__builtin_ctz(bitfield)] : nullptr;
#else
    int lo, mid, hi;
    lo = 0;
    hi = n_children_;
    while (lo < hi) {
        mid = (lo + hi) / 2;
        if (partial_key < keys_[mid]) {
            hi = mid;
        } else if (partial_key > keys_[mid]) {
            lo = mid + 1;
        } else {
            return &children_[mid];
        }
    }
    return nullptr;
#endif
}


template <class T> void node_16<T>::set_child(uint8_t partial_key, node<T> *child) {
    /* determine index for child */
    int child_i;
    for (int i = this->n_children_ - 1;; --i) {
        if (i >= 0 && partial_key < this->keys_[i]) {
            /* move existing sibling to the right */
            this->keys_[i + 1] = this->keys_[i];
            this->children_[i + 1] = this->children_[i];
        } else {
            child_i = i + 1;
            break;
        }
    }

    this->keys_[child_i] = partial_key;
    this->children_[child_i] = child;
    ++n_children_;
}

template <class T> node<T> *node_16<T>::del_child(uint8_t partial_key) {
    node<T> *child_to_delete = nullptr;
    for (int i = 0; i < n_children_; ++i) {
        if (child_to_delete == nullptr && partial_key == keys_[i]) {
            child_to_delete = children_[i];
        }
        if (child_to_delete != nullptr) {
            /* move existing sibling to the left */
            keys_[i] = i < n_children_ - 1 ? keys_[i + 1] : 0;
            children_[i] = i < n_children_ - 1 ? children_[i + 1] : nullptr;
        }
    }
    if (child_to_delete != nullptr) {
        --n_children_;
    }
    return child_to_delete;
}

template <class T> inner_node<T> *node_16<T>::grow() {
    auto new_node = new node_48<T>();
    new_node->prefix_ = this->prefix_;
    new_node->prefix_len_ = this->prefix_len_;
    new_node->n_children_ = this->n_children_;
    std::copy(this->children_, this->children_ + this->n_children_, new_node->children_);
    for (int i = 0; i < n_children_; ++i) {
        new_node->indexes_[(uint8_t) this->keys_[i]] = i;
    }
    delete this;
    return new_node;
}

template <class T> inner_node<T> *node_16<T>::shrink() {
    auto new_node = new node_4<T>();
    new_node->prefix_ = this->prefix_;
    new_node->prefix_len_ = this->prefix_len_;
    new_node->n_children_ = this->n_children_;
    std::copy(this->keys_, this->keys_ + this->n_children_, new_node->keys_);
    std::copy(this->children_, this->children_ + this->n_children_, new_node->children_);
    delete this;
    return new_node;
}

template <class T> bool node_16<T>::is_full() const {
    return n_children_ == 16;
}

template <class T> bool node_16<T>::is_underfull() const {
    return n_children_ == 4;
}

template <class T> uint8_t node_16<T>::next_partial_key(uint8_t partial_key) const {
    for (int i = 0; i < n_children_; ++i) {
        if (keys_[i] >= partial_key) {
            return keys_[i];
        }
    }
    throw std::out_of_range("provided partial key does not have a successor");
}

template <class T> uint8_t node_16<T>::prev_partial_key(uint8_t partial_key) const {
    for (int i = n_children_ - 1; i >= 0; --i) {
        if (keys_[i] <= partial_key) {
            return keys_[i];
        }
    }
    throw std::out_of_range("provided partial key does not have a predecessor");
}

template <class T> int node_16<T>::n_children() const { return n_children_; }

} // namespace art

#endif
