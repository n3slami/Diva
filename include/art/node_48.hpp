/**
 * @file node_48 header
 * @author Rafael Kallis <rk@rafaelkallis.com>
 */

#ifndef ART_NODE_48_HPP
#define ART_NODE_48_HPP

#include "inner_node.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>

namespace art {

template <class T> class node_16;
template <class T> class node_16_valued;
template <class T> class node_256;
template <class T> class node_256_valued;

template <class T> class node_48_valued;

template <class T> class node_48 : public inner_node<T> {
    friend class node_16<T>;
    friend class node_256<T>;

    friend class node_16_valued<T>;
    friend class node_48_valued<T>;
    friend class node_256_valued<T>;
public:
    node_48();

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
        node_48_valued<T> *res = new node_48_valued<T>();
        res->prefix_ = this->prefix_;
        res->prefix_len_ = this->prefix_len_;
        res->n_children_ = this->n_children_;
        memcpy(res->indexes_, this->indexes_, sizeof(res->indexes_));
        memcpy(res->children_, this->children_, sizeof(res->children_));
        res->value_ = value;
        delete this;
        return res;
    }

    inner_node<T> *remove_value() override {
        throw std::runtime_error("cannot remove value from a node without a value");
    };


private:
    static const uint8_t EMPTY;

    uint8_t n_children_ = 0;
    uint8_t indexes_[256];
    node<T> *children_[48];
};


template <class T> class node_48_valued : public node_48<T> {
public:
    T *get_value() override {
        return reinterpret_cast<T *>(&value_);
    };

    inner_node<T> *grow() override {
        node_256_valued<T> *new_node = new node_256_valued<T>();
        new_node->prefix_ = this->prefix_;
        new_node->prefix_len_ = this->prefix_len_;
        uint8_t index;
        for (int partial_key = 0; partial_key < 256; ++partial_key) {
            index = this->indexes_[partial_key];
            if (index != node_48<T>::EMPTY) {
                new_node->set_child(partial_key, this->children_[index]);
            }
        }
        new_node->value_ = this->value_;
        delete this;
        return new_node;
    }

    inner_node<T> *shrink() override {
        node_16_valued<T> *new_node = new node_16_valued<T>();
        new_node->prefix_ = this->prefix_;
        new_node->prefix_len_ = this->prefix_len_;
        uint8_t index;
        for (int partial_key = 0; partial_key < 256; ++partial_key) {
            index = this->indexes_[partial_key];
            if (index != node_48<T>::EMPTY) {
                new_node->set_child(partial_key, this->children_[index]);
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

    inner_node<T> *remove_value() override {
        node_48<T> *res = new node_48<T>();
        res->prefix_ = this->prefix_;
        res->prefix_len_ = this->prefix_len_;
        res->n_children_ = this->n_children_;
        memcpy(res->indexes_, this->indexes_, sizeof(res->indexes_));
        memcpy(res->children_, this->children_, sizeof(res->children_));
        delete this;
        return res;
    }

    T value_;
};


template <class T> node_48<T>::node_48() {
    std::fill(this->indexes_, this->indexes_ + 256, node_48::EMPTY);
    std::fill(this->children_, this->children_ + 48, nullptr);
}

template <class T> node<T> **node_48<T>::find_child(uint8_t partial_key) {
    // TODO(rafaelkallis): direct lookup instead of temp save?
    uint8_t index = indexes_[partial_key];
    return node_48::EMPTY != index ? &children_[index] : nullptr;
}

template <class T> void node_48<T>::set_child(uint8_t partial_key, node<T> *child) {
    // TODO(rafaelkallis): pick random starting entry in order to increase
    // performance? i.e. for (int i = random([0,48)); i != (i-1) % 48; i = (i+1) %
    // 48){}

    /* find empty child entry */
    for (int i = 0; i < 48; ++i) {
        if (children_[i] == nullptr) {
            indexes_[partial_key] = (uint8_t) i;
            children_[i] = child;
            break;
        }
    }
    ++n_children_;
}

template <class T> node<T> *node_48<T>::del_child(uint8_t partial_key) {
    node<T> *child_to_delete = nullptr;
    uint8_t index = indexes_[partial_key];
    if (index != node_48::EMPTY) {
        child_to_delete = children_[index];
        indexes_[partial_key] = node_48::EMPTY;
        children_[index] = nullptr;
        --n_children_;
    }
    return child_to_delete;
}

template <class T> inner_node<T> *node_48<T>::grow() {
    auto new_node = new node_256<T>();
    new_node->prefix_ = this->prefix_;
    new_node->prefix_len_ = this->prefix_len_;
    uint8_t index;
    for (int partial_key = 0; partial_key < 256; ++partial_key) {
        index = indexes_[partial_key];
        if (index != node_48::EMPTY) {
            new_node->set_child(partial_key, children_[index]);
        }
    }
    delete this;
    return new_node;
}

template <class T> inner_node<T> *node_48<T>::shrink() {
    auto new_node = new node_16<T>();
    new_node->prefix_ = this->prefix_;
    new_node->prefix_len_ = this->prefix_len_;
    uint8_t index;
    for (int partial_key = 0; partial_key < 256; ++partial_key) {
        index = indexes_[partial_key];
        if (index != node_48::EMPTY) {
            new_node->set_child(partial_key, children_[index]);
        }
    }
    delete this;
    return new_node;
}

template <class T> bool node_48<T>::is_full() const {
    return n_children_ == 48;
}

template <class T> bool node_48<T>::is_underfull() const {
    return n_children_ == 16;
}

template <class T> const uint8_t node_48<T>::EMPTY = 48;

template <class T> uint8_t node_48<T>::next_partial_key(uint8_t partial_key) const {
    while (true) {
        if (indexes_[partial_key] != node_48<T>::EMPTY) {
            return partial_key;
        }
        if (partial_key == 255) {
            throw std::out_of_range("provided partial key does not have a successor");
        }
        ++partial_key;
    }
}

template <class T> uint8_t node_48<T>::prev_partial_key(uint8_t partial_key) const {
    while (true) {
        if (indexes_[partial_key] != node_48<T>::EMPTY) {
            return partial_key;
        }
        if (partial_key == 0) {
            throw std::out_of_range(
                    "provided partial key does not have a predecessor");
        }
        --partial_key;
    }
}

template <class T> int node_48<T>::n_children() const { return n_children_; }

} // namespace art

#endif
