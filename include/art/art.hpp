/**
 * @file adaptive radix tree
 * @author Rafael Kallis <rk@rafaelkallis.com>
 */

#ifndef ART_ART_HPP
#define ART_ART_HPP

#include "leaf_node.hpp"
#include "inner_node.hpp"
#include "node.hpp"
#include "node_4.hpp"
#include "tree_it.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stack>

namespace art {

template <class T> class art {
public:
    ~art();

    /**
     * Finds the value associated with the given key.
     *
     * @param key - The key to find.
     * @return the value associated with the key or a default constructed value.
     */
    T get(const uint8_t *key) const;

    /**
     * Finds the value associated with the given key.
     *
     * @param key - The key to find.
     * @param key_len - The length of the key string.
     * @return the value associated with the key or a default constructed value.
     */
    T get(const uint8_t *key, const uint32_t key_len) const;


    /**
     * Associates the given key with the given value.
     * If another value is already associated with the given key,
     * since the method consumer is the resource owner.
     *
     * @param key - The key to associate with the value.
     * @param value - The value to be associated with the key.
     * @return a nullptr if no other value is associated with the or the
     * previously associated value.
     */
    T set(const uint8_t *key, T value);

    /**
     * Associates the given key with the given value.
     * If another value is already associated with the given key,
     * since the method consumer is the resource owner.
     *
     * @param key - The key to associate with the value.
     * @param key_len - The length of the key string.
     * @param value - The value to be associated with the key.
     * @return a nullptr if no other value is associated with the or the
     * previously associated value.
     */
    T set(const uint8_t *key, const uint32_t key_len, T value);


    /**
     * Deletes the given key and returns it's associated value.
     * The associated value is returned,
     * since the method consumer is the resource owner.
     * If no value is associated with the given key, nullptr is returned.
     *
     * @param key - The key to delete. 
     * @return the values assciated with the key or a nullptr otherwise.
     */
    T del(const uint8_t *key);

    /**
     * Deletes the given key and returns it's associated value.
     * The associated value is returned,
     * since the method consumer is the resource owner.
     * If no value is associated with the given key, nullptr is returned.
     *
     * @param key - The key to delete. 
     * @param key_len - The length of the key string.
     * @return the values assciated with the key or a nullptr otherwise.
     */
    T del(const uint8_t *key, const uint32_t key_len);


    /**
     * Forward iterator that traverses the tree in lexicographic order.
     */
    tree_it<T> begin() const;

    /**
     * Forward iterator that traverses the tree in lexicographic order starting
     * from the provided key.
     */
    tree_it<T> begin(const uint8_t *key) const;

    /**
     * Forward iterator that traverses the tree in lexicographic order starting
     * from the provided key.
     */
    tree_it<T> begin(const uint8_t *key, const uint32_t key_len) const;


    /**
     * Iterator to the end of the lexicographic order.
     */
    tree_it<T> end() const;

private:
    node<T> *root_ = nullptr;
};

template <class T> art<T>::~art() {
    if (root_ == nullptr) {
        return;
    }
    std::stack<node<T> *> node_stack;
    node_stack.push(root_);
    node<T> *cur;
    inner_node<T> *cur_inner;
    child_it<T> it, it_end;
    while (!node_stack.empty()) {
        cur = node_stack.top();
        node_stack.pop();
        if (!cur->is_leaf()) {
            cur_inner = static_cast<inner_node<T>*>(cur);
            for (it = cur_inner->begin(), it_end = cur_inner->end(); it != it_end; ++it) {
                node_stack.push(*cur_inner->find_child(*it));
            }
        }
        if (cur->prefix_ != nullptr) {
            delete[] cur->prefix_;
        }
        delete cur;
    }
}

template <class T> 
T art<T>::get(const uint8_t *key) const {
    return get(key, std::strlen(reinterpret_cast<const char *>(key)));
}

template <class T> 
T art<T>::get(const uint8_t *key, const uint32_t key_len) const {
    node<T> *cur = root_, **child;
    int depth = 0;
    while (cur != nullptr) {
        if (cur->prefix_len_ != cur->check_prefix(key + depth, key_len - depth)) {
            /* prefix mismatch */
            return T{};
        }
        if (cur->prefix_len_ == key_len - depth) {
            /* exact match */
            if (cur->is_leaf())
                return reinterpret_cast<leaf_node<T>*>(cur)->value_;
            inner_node<T> *cur_inner = reinterpret_cast<inner_node<T> *>(cur);
            return cur_inner->get_value() != nullptr ? *cur_inner->get_value() : T{};
        }
        if (cur->is_leaf())
            return T{};
        child = reinterpret_cast<inner_node<T> *>(cur)->find_child(key[depth + cur->prefix_len_]);
        depth += (cur->prefix_len_ + 1);
        cur = child != nullptr ? *child : nullptr;
    }
    return T{};
}


template <class T> 
T art<T>::set(const uint8_t *key, T value) {
    return set(key, std::strlen(reinterpret_cast<const char *>(key)), value);
}

template <class T> 
T art<T>::set(const uint8_t *key, const uint32_t key_len, T value) {
    int depth = 0, prefix_match_len;
    if (root_ == nullptr) {
        root_ = new leaf_node<T>(value);
        root_->prefix_ = new uint8_t[key_len];
        std::copy(key, key + key_len, root_->prefix_);
        root_->prefix_len_ = key_len;
        return T{};
    }

    node<T> **cur = &root_, **child;
    inner_node<T> **cur_inner = nullptr;
    uint8_t child_partial_key;
    bool is_prefix_match;

    while (true) {
        /* number of bytes of the current node's prefix that match the key */
        prefix_match_len = (**cur).check_prefix(key + depth, key_len - depth);

        /* true if the current node's prefix matches with a part of the key */
        is_prefix_match = (std::min<int>((**cur).prefix_len_, key_len - depth)) == prefix_match_len;

        if (is_prefix_match) {
            if ((**cur).prefix_len_ == key_len - depth) {
                /* exact match:
                 * => "replace"
                 * => replace value of current node.
                 * => return old value to caller to handle.
                 *        _                             _
                 *        |                             |
                 *       (aa)                          (aa)
                 *    a /    \ b     +[aaaaa,v3]    a /    \ b
                 *     /      \      ==========>     /      \
                 * *(aa)->v1  ()->v2             *(aa)->v3  ()->v2
                 *
                 */

                T old_value {};
                if ((**cur).is_leaf()) {
                    auto cur_leaf = static_cast<leaf_node<T>*>(*cur);
                    old_value = cur_leaf->value_;
                    cur_leaf->value_ = value;
                }
                else {
                    cur_inner = reinterpret_cast<inner_node<T> **>(cur);
                    if ((**cur_inner).get_value() != nullptr)
                        old_value = *((**cur_inner).get_value());
                    *cur = (**cur_inner).set_value(value);
                }
                return old_value;
            }
            else if (((**cur).prefix_len_ < key_len - depth && (**cur).is_leaf())) {
                /* prefix match:
                 * => convert leaf into inner node and add new key as its child.
                 *        _                             _
                 *        |                             |
                 *       (aa)->v1    +[aaaaa,v2]       (aa)->v1
                 *                   ==========>        |
                 *                                     (aaa)->v2
                 *
                 */

                auto new_node = new leaf_node<T>(value);
                new_node->prefix_ = new uint8_t[key_len - depth - (**cur).prefix_len_ - 1];
                std::copy(key + depth + (**cur).prefix_len_ + 1, key + key_len, new_node->prefix_);
                new_node->prefix_len_ = key_len - depth - (**cur).prefix_len_ - 1;

                leaf_node<T> *cur_leaf = reinterpret_cast<leaf_node<T> *>(*cur);
                auto new_parent = new node_4_valued<T>();
                new_parent->prefix_ = cur_leaf->prefix_;
                new_parent->prefix_len_ = cur_leaf->prefix_len_;
                new_parent->set_value(cur_leaf->value_);
                new_parent->set_child(key[depth + cur_leaf->prefix_len_], new_node);

                delete cur_leaf;

                *cur = new_parent;
                return T{};
            }
            else if ((**cur).prefix_len_ > key_len - depth) {
                /* prefix match:
                 * => new prefix parent node.
                 * => new inner node with value to insert.
                 * => current node becomes child of new parent node.
                 *
                 *        |                        |
                 *      *(aa)                    +(a)->v3
                 *    a /    \ b     +[a,v3]     a |   
                 *     /      \      =======>      |
                 *  (aa)->v1  ()->v2             *()
                 *                             a /  \ b
                 *                              /    \
                 *                          (aa)->v1 ()->v2
                 *                          /|\      /|\
                 */

                auto new_parent = new node_4_valued<T>();
                new_parent->prefix_ = new uint8_t[prefix_match_len];
                std::copy((**cur).prefix_, (**cur).prefix_ + prefix_match_len, new_parent->prefix_);
                new_parent->prefix_len_ = prefix_match_len;
                new_parent->set_child((**cur).prefix_[prefix_match_len], *cur);
                new_parent->value_ = value;

                auto old_prefix = (**cur).prefix_;
                auto old_prefix_len = (**cur).prefix_len_;
                (**cur).prefix_ = new uint8_t[old_prefix_len - prefix_match_len - 1];
                (**cur).prefix_len_ = old_prefix_len - prefix_match_len - 1;
                std::copy(old_prefix + prefix_match_len + 1, old_prefix + old_prefix_len, (**cur).prefix_);
                delete old_prefix;

                *cur = new_parent;
                return T{};
            }
        }
        else {
            /* prefix mismatch:
             * => new parent node with common prefix and no associated value.
             * => new node with value to insert.
             * => current and new node become children of new parent node.
             *
             *        |                        |
             *      *(aa)                    +(a)->Ø
             *    a /    \ b     +[ab,v3]  a /   \ b
             *     /      \      =======>   /     \
             *  (aa)->v1  ()->v2          *()->Ø +()->v3
             *                          a /   \ b
             *                           /     \
             *                        (aa)->v1 ()->v2
             *                        /|\      /|\
             */

            auto new_parent = new node_4<T>();
            new_parent->prefix_ = new uint8_t[prefix_match_len];
            std::copy((**cur).prefix_, (**cur).prefix_ + prefix_match_len,
                    new_parent->prefix_);
            new_parent->prefix_len_ = prefix_match_len;
            new_parent->set_child((**cur).prefix_[prefix_match_len], *cur);

            // TODO(rafaelkallis): shrink?
            /* memmove((**cur).prefix_, (**cur).prefix_ + prefix_match_len + 1, */
            /*         (**cur).prefix_len_ - prefix_match_len - 1); */
            /* (**cur).prefix_len_ -= prefix_match_len + 1; */

            auto old_prefix = (**cur).prefix_;
            auto old_prefix_len = (**cur).prefix_len_;
            (**cur).prefix_ = new uint8_t[old_prefix_len - prefix_match_len - 1];
            (**cur).prefix_len_ = old_prefix_len - prefix_match_len - 1;
            std::copy(old_prefix + prefix_match_len + 1, old_prefix + old_prefix_len,
                    (**cur).prefix_);
            delete old_prefix;

            auto new_node = new leaf_node<T>(value);
            new_node->prefix_ = new uint8_t[key_len - depth - prefix_match_len - 1];
            std::copy(key + depth + prefix_match_len + 1, key + key_len,
                    new_node->prefix_);
            new_node->prefix_len_ = key_len - depth - prefix_match_len - 1;
            new_parent->set_child(key[depth + prefix_match_len], new_node);

            *cur = new_parent;
            return T{};
        }

        /* must be inner node */
        cur_inner = reinterpret_cast<inner_node<T>**>(cur);
        child_partial_key = key[depth + (**cur).prefix_len_];
        child = (**cur_inner).find_child(child_partial_key);

        if (child == nullptr) {
            /*
             * no child associated with the next partial key.
             * => create new node with value to insert.
             * => new node becomes current node's child.
             *
             *      *(aa)->Ø              *(aa)->Ø
             *    a /        +[aab,v2]  a /    \ b
             *     /         ========>   /      \
             *   (a)->v1               (a)->v1 +()->v2
             */

            if ((**cur_inner).is_full()) {
                *cur_inner = (**cur_inner).grow();
            }

            auto new_node = new leaf_node<T>(value);
            new_node->prefix_ = new uint8_t[key_len - depth - (**cur).prefix_len_ - 1];
            std::copy(key + depth + (**cur).prefix_len_ + 1, key + key_len,
                    new_node->prefix_);
            new_node->prefix_len_ = key_len - depth - (**cur).prefix_len_ - 1;
            (**cur_inner).set_child(child_partial_key, new_node);
            return T{};
        }

        /* propagate down and repeat:
         *
         *     *(aa)->Ø                   (aa)->Ø
         *   a /    \ b    +[aaba,v3]  a /    \ b     repeat
         *    /      \     =========>   /      \     ========>  ...
         *  (a)->v1  ()->v2           (a)->v1 *()->v2
         */

        depth += (**cur).prefix_len_ + 1;
        cur = child;
    }
}

template <class T> 
T art<T>::del(const uint8_t *key) {
    return del(key, std::strlen(reinterpret_cast<const char *>(key)));
}

template <class T> 
T art<T>::del(const uint8_t *key, const uint32_t key_len) {
    int depth = 0;

    if (root_ == nullptr) {
        return T{};
    }

    /* pointer to parent, current and child node */
    node<T> **cur = &root_;
    inner_node<T> **par = nullptr;

    /* partial key of current and child node */
    uint8_t cur_partial_key = 0;

    while (cur != nullptr) {
        if ((**cur).prefix_len_ !=
                (**cur).check_prefix(key + depth, key_len - depth)) {
            /* prefix mismatch => key doesn't exist */

            return T{};
        }

        if (key_len == depth + (**cur).prefix_len_) {
            /* exact match */
            if (!(**cur).is_leaf()) {
                inner_node<T> **inner_cur = reinterpret_cast<inner_node<T> **>(cur);
                if ((**inner_cur).get_value() != nullptr) {
                    T res = *(**inner_cur).get_value();
                    auto n_children = (**inner_cur).n_children();
                    if (n_children == 1) {
                        /* find child */
                        auto partial_key = (**inner_cur).next_partial_key(0);
                        auto child = *(**inner_cur).find_child(partial_key);

                        auto old_prefix = child->prefix_;
                        auto old_prefix_len = child->prefix_len_;

                        child->prefix_ = new uint8_t[(**inner_cur).prefix_len_ + 1 + old_prefix_len];
                        child->prefix_len_ = (**inner_cur).prefix_len_ + 1 + old_prefix_len;
                        std::copy((**inner_cur).prefix_, (**inner_cur).prefix_ + (**inner_cur).prefix_len_, child->prefix_);
                        child->prefix_[(**inner_cur).prefix_len_] = partial_key;
                        std::copy(old_prefix, old_prefix + old_prefix_len, child->prefix_ + (**inner_cur).prefix_len_ + 1);
                        if (old_prefix != nullptr) {
                            delete[] old_prefix;
                        }
                        if ((**inner_cur).prefix_ != nullptr) {
                            delete[](**inner_cur).prefix_;
                        }
                        delete (*inner_cur);
                        *cur = child;
                    }
                    else {
                        *cur = (**inner_cur).remove_value();
                    }
                    return res;
                }
                return T{};
            }
            auto value = reinterpret_cast<leaf_node<T>*>(*cur)->value_;
            auto n_siblings = par != nullptr ? (**par).n_children() - 1 : 0;

            if (n_siblings == 0) {
                if (cur == &root_) {
                    if ((**cur).is_leaf()) {
                        /*
                         * => must be root node
                         * => delete root node
                         *
                         *     |       -[aa]     |
                         *     |       =======>  
                         *   *(aa)->v2
                         */

                        if ((**cur).prefix_ != nullptr) {
                            delete[](**cur).prefix_;
                        }
                        delete (*cur);
                        *cur = nullptr;
                    }
                    else
                        *cur = (**reinterpret_cast<inner_node<T> **>(cur)).remove_value();
                }
                else {
                    /*
                     * => parent must a valued inner node
                     * => delete parent node replace it with the leaf node
                     *
                     *     |                 |
                     *    (aa)->v1          (aa)->v1
                     *     | a     -[aaaaa]
                     *     |       =======>
                     *   *(aa)->v2
                     */

                    leaf_node<T> **cur_leaf = reinterpret_cast<leaf_node<T> **>(cur);
                    auto old_prefix = (**cur).prefix_;
                    auto old_par_ptr = *par;

                    (**cur_leaf).prefix_ = (**par).prefix_;
                    (**cur_leaf).prefix_len_ = (**par).prefix_len_;
                    (**cur_leaf).value_ = *((**par).get_value());

                    if (old_prefix != nullptr) {
                        delete[] old_prefix;
                    }
                    node<T> **converted_par = reinterpret_cast<node<T> **>(par);
                    *converted_par = *cur;
                }
            } else if (n_siblings == 1 && (**par).get_value() == nullptr) {
                /* => delete leaf node
                 * => replace parent with sibling
                 *
                 *        |a                         |a
                 *        |                          |
                 *       (aa)        -"aaaaabaa"     |
                 *    a /    \ b     ==========>    /
                 *     /      \                    /
                 *  (aa)->v1 *()->v2             (aaaaa)->v1
                 *  /|\                            /|\
                 */

                /* find sibling */
                auto sibling_partial_key = (**par).next_partial_key(0);
                if (sibling_partial_key == cur_partial_key) {
                    sibling_partial_key = (**par).next_partial_key(cur_partial_key + 1);
                }
                auto sibling = *(**par).find_child(sibling_partial_key);

                auto old_prefix = sibling->prefix_;
                auto old_prefix_len = sibling->prefix_len_;

                sibling->prefix_ = new uint8_t[(**par).prefix_len_ + 1 + old_prefix_len];
                sibling->prefix_len_ = (**par).prefix_len_ + 1 + old_prefix_len;
                std::copy((**par).prefix_, (**par).prefix_ + (**par).prefix_len_, sibling->prefix_);
                sibling->prefix_[(**par).prefix_len_] = sibling_partial_key;
                std::copy(old_prefix, old_prefix + old_prefix_len, sibling->prefix_ + (**par).prefix_len_ + 1);
                if (old_prefix != nullptr) {
                    delete[] old_prefix;
                }
                if ((**cur).prefix_ != nullptr) {
                    delete[](**cur).prefix_;
                }
                delete (*cur);
                if ((**par).prefix_ != nullptr) {
                    delete[](**par).prefix_;
                }
                delete (*par);

                /* this looks crazy, but I know what I'm doing */
                *par = static_cast<inner_node<T>*>(sibling);

            } else if (n_siblings == 1) {
                /* => delete leaf node only, as parent has a value
                 *
                 *        |a                         |a
                 *        |                          |
                 *       (aa)->v0    -"aaaaabaa"     (aa)->v0
                 *    a /    \ b     ==========>  a /
                 *     /      \                    /
                 *  (aa)->v1 *()->v2             (aa)->v1
                 *  /|\                          /|\
                 */

                if ((**cur).prefix_ != nullptr) {
                    delete[](**cur).prefix_;
                }
                delete (*cur);
                (**par).del_child(cur_partial_key);
            } else /* if (n_siblings > 1) */ {
                /* => delete leaf node
                 *
                 *        |a                         |a
                 *        |                          |
                 *       (aa)        -"aaaaabaa"    (aa)   
                 *    a / |  \ b     ==========> a / |
                 *     /  |   \                   /  |
                 *           *()->v1
                 */

                if ((**cur).prefix_ != nullptr) {
                    delete[](**cur).prefix_;
                }
                delete (*cur);
                (**par).del_child(cur_partial_key);
                if ((**par).is_underfull()) {
                    *par = (**par).shrink();
                }
            }

            return value;
        }

        /* propagate down and repeat */
        cur_partial_key = key[depth + (**cur).prefix_len_];
        depth += (**cur).prefix_len_ + 1;
        par = reinterpret_cast<inner_node<T>**>(cur);
        cur = (**par).find_child(cur_partial_key);
    }
    return T{};
}

template <class T> tree_it<T> art<T>::begin() const {
    return tree_it<T>::min(this->root_);
}

template <class T> tree_it<T> art<T>::begin(const uint8_t *key) const {
    return tree_it<T>::greater_equal(this->root_, key);
}

template <class T> tree_it<T> art<T>::begin(const uint8_t *key, const uint32_t key_len) const {
    return tree_it<T>::greater_equal(this->root_, key, key_len);
}

template <class T> tree_it<T> art<T>::end() const { 
    return tree_it<T>(); 
}

} // namespace art

#endif
