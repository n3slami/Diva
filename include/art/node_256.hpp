/**
 * @file node_256 header
 * @author Rafael Kallis <rk@rafaelkallis.com>
 */

#ifndef ART_NODE_256_HPP
#define ART_NODE_256_HPP

#include "inner_node.hpp"
#include <array>
#include <cstdint>
#include <stdexcept>

namespace art {

template <class T> class node_48;
template <class T> class node_48_valued;

template <class T> class node_256_valued;

template <class T> class node_256 : public inner_node<T> {
    friend class node_48<T>;
    friend class node_48_valued<T>;
    friend class node_256_valued<T>;
public:
    node_256();

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
        node_256_valued<T> *res = new node_256_valued<T>();
        res->prefix_ = this->prefix_;
        res->prefix_len_ = this->prefix_len_;
        res->n_children_ = this->n_children_;
        res->children_ = this->children_;
        res->value_ = value;
        delete this;
        return res;
    }

    inner_node<T> *remove_value() override {
        throw std::runtime_error("cannot remove value from a node without a value");
    };

private:
    uint16_t n_children_ = 0;
    std::array<node<T> *, 256> children_;
};


template <class T> class node_256_valued : public node_256<T> {
public:
    T *get_value() override {
        return reinterpret_cast<T *>(&value_);
    };

    inner_node<T> *shrink() override {
        node_48_valued<T> *new_node = new node_48_valued<T>();
        new_node->prefix_ = this->prefix_;
        new_node->prefix_len_ = this->prefix_len_;
        for (int partial_key = 0; partial_key < 256; ++partial_key) {
            if (this->children_[partial_key] != nullptr) {
                new_node->set_child(partial_key, this->children_[partial_key]);
            }
        }
        new_node->value_ = this->value_;
        delete this;
        return new_node;
    }

    inner_node<T> *set_value(T &value) override {
        this->value_ = value;
        return this;
    };

    inner_node<T> *remove_value() {
        node_256<T> *res = new node_256<T>();
        res->prefix_ = this->prefix_;
        res->prefix_len_ = this->prefix_len_;
        res->n_children_ = this->n_children_;
        res->children_ = this->children_;
        delete this;
        return res;
    }

    T value_;
};


template <class T> node_256<T>::node_256() { children_.fill(nullptr); }

template <class T> node<T> **node_256<T>::find_child(uint8_t partial_key) {
    return children_[partial_key] != nullptr ? &children_[partial_key]
        : nullptr;
}

template <class T> void node_256<T>::set_child(uint8_t partial_key, node<T> *child) {
    children_[partial_key] = child;
    ++n_children_;
}

template <class T> node<T> *node_256<T>::del_child(uint8_t partial_key) {
    node<T> *child_to_delete = children_[partial_key];
    if (child_to_delete != nullptr) {
        children_[partial_key] = nullptr;
        --n_children_;
    }
    return child_to_delete;
}

template <class T> inner_node<T> *node_256<T>::grow() {
    throw std::runtime_error("node_256 cannot grow");
}

template <class T> inner_node<T> *node_256<T>::shrink() {
    auto new_node = new node_48<T>();
    new_node->prefix_ = this->prefix_;
    new_node->prefix_len_ = this->prefix_len_;
    for (int partial_key = 0; partial_key < 256; ++partial_key) {
        if (children_[partial_key] != nullptr) {
            new_node->set_child(partial_key, children_[partial_key]);
        }
    }
    delete this;
    return new_node;
}

template <class T> bool node_256<T>::is_full() const {
    return n_children_ == 256;
}

template <class T> bool node_256<T>::is_underfull() const {
    return n_children_ == 48;
}

template <class T> uint8_t node_256<T>::next_partial_key(uint8_t partial_key) const {
    while (true) {
        if (children_[partial_key] != nullptr) {
            return partial_key;
        }
        if (partial_key == 255) {
            throw std::out_of_range("provided partial key does not have a successor");
        }
        ++partial_key;
    }
}

template <class T> uint8_t node_256<T>::prev_partial_key(uint8_t partial_key) const {
    while (true) {
        if (children_[partial_key] != nullptr) {
            return partial_key;
        }
        if (partial_key == 0) {
            throw std::out_of_range(
                    "provided partial key does not have a predecessor");
        }
        --partial_key;
    }
}

template <class T> int node_256<T>::n_children() const { return n_children_; }

} // namespace art

#endif
