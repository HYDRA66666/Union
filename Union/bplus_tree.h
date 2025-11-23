#pragma once
#include "pch.h"

namespace HYDRA15::Union::archivist
{
    // 用于数据库索引的 B+ 树
    // 使用自定义比较器判断元素顺序
    // ** AI 生成的代码，未经测试
    template<typename T>
    class bplus_tree {
    public:
        using comparator_t = std::function<std::strong_ordering(const T&, const T&)>;

        explicit bplus_tree(comparator_t comp, int maxKeysPerLeaf = 32)
            : comp_(std::move(comp)), max_keys_(std::max(3, maxKeysPerLeaf))
        {
            min_keys_ = (max_keys_ + 1) / 2; // minimal keys per node (except root)
            root_ = new Node(true);
        }

        ~bplus_tree() {
            delete_subtree(root_);
        }

        // 插入元素（若重复元素仍插入，行为可视为 multiset；若想 set 行为，先 find 再决定）
        void insert(const T& key) {
            InsertResult res = insert_recursive(root_, key);
            if (res.new_right) {
                // 根分裂 -> 新根
                Node* new_root = new Node(false);
                new_root->keys.push_back(res.promoted_key);
                new_root->children.push_back(root_);
                new_root->children.push_back(res.new_right);
                root_ = new_root;
            }
        }

        // 删除元素（仅删除一个匹配项），返回是否删除成功
        bool erase(const T& key) {
            bool removed = erase_recursive(root_, key);
            // 如果根是内部且没有键，压缩高度
            if (!root_->is_leaf && root_->keys.empty()) {
                Node* old = root_;
                root_ = root_->children.empty() ? new Node(true) : root_->children[0];
                // 避免 double delete root_ when it becomes child: only delete old
                old->children.clear();
                delete old;
            }
            return removed;
        }

        // 精确查找（使用比较器判等），返回第一个匹配项
        std::optional<T> find(const T& key) const {
            Node* leaf = find_leaf(root_, key);
            if (!leaf) return std::nullopt;
            auto it = lower_bound_in_keys(leaf->keys, key);
            if (it != leaf->keys.end() && comp_(*it, key) == std::strong_ordering::equal) {
                return *it;
            }
            return std::nullopt;
        }

        // 范围查找（闭区间 [low, high]），返回按顺序的元素副本
        std::vector<T> range_search(const T& low, const T& high) const {
            std::vector<T> result;
            if (comp_(high, low) == std::strong_ordering::less) return result; // empty if high < low
            Node* leaf = find_leaf_for_lower_bound(root_, low);
            while (leaf) {
                for (const auto& k : leaf->keys) {
                    if (comp_(k, low) == std::strong_ordering::less) continue;
                    if (comp_(k, high) == std::strong_ordering::greater) return result;
                    result.push_back(k);
                }
                leaf = leaf->next;
            }
            return result;
        }

    private:
        struct Node {
            bool is_leaf;
            std::vector<T> keys;
            std::vector<Node*> children; // for internal nodes: children.size() == keys.size() + 1
            Node* next = nullptr; // for leaf nodes
            explicit Node(bool leaf) : is_leaf(leaf) {}
        };

        Node* root_ = nullptr;
        comparator_t comp_;
        int max_keys_;
        int min_keys_;

        // Helper - compare key ordering
        static bool key_less(const comparator_t& cmp, const T& a, const T& b) {
            return cmp(a, b) == std::strong_ordering::less;
        }

        static bool key_equal(const comparator_t& cmp, const T& a, const T& b) {
            return cmp(a, b) == std::strong_ordering::equal;
        }

        // Find leaf that may contain key
        Node* find_leaf(Node* node, const T& key) const {
            if (!node) return nullptr;
            Node* cur = node;
            while (!cur->is_leaf) {
                size_t i = 0;
                while (i < cur->keys.size() && comp_(cur->keys[i], key) == std::strong_ordering::less) ++i;
                cur = cur->children[i];
            }
            return cur;
        }

        // Find leaf for lower_bound (first leaf that could contain >= key)
        Node* find_leaf_for_lower_bound(Node* node, const T& key) const {
            return find_leaf(node, key);
        }

        // lower_bound within vector<T> using comparator
        typename std::vector<T>::const_iterator lower_bound_in_keys(const std::vector<T>& v, const T& key) const {
            return std::lower_bound(v.begin(), v.end(), key,
                [this](const T& a, const T& b) { return comp_(a, b) == std::strong_ordering::less; });
        }
        typename std::vector<T>::iterator lower_bound_in_keys(std::vector<T>& v, const T& key) const {
            return std::lower_bound(v.begin(), v.end(), key,
                [this](const T& a, const T& b) { return comp_(a, b) == std::strong_ordering::less; });
        }

        // Insert result used to propagate splits
        struct InsertResult {
            Node* new_right = nullptr;
            T promoted_key{};
        };

        InsertResult insert_recursive(Node* node, const T& key) {
            if (node->is_leaf) {
                auto it = lower_bound_in_keys(node->keys, key);
                node->keys.insert(it, key);
                if ((int)node->keys.size() > max_keys_) {
                    // split leaf
                    Node* new_leaf = new Node(true);
                    size_t total = node->keys.size();
                    size_t split = total / 2;
                    new_leaf->keys.assign(node->keys.begin() + split, node->keys.end());
                    node->keys.erase(node->keys.begin() + split, node->keys.end());
                    new_leaf->next = node->next;
                    node->next = new_leaf;
                    InsertResult r;
                    r.new_right = new_leaf;
                    r.promoted_key = new_leaf->keys.front();
                    return r;
                }
                else {
                    return InsertResult{};
                }
            }
            else {
                // internal node
                size_t idx = 0;
                while (idx < node->keys.size() && comp_(node->keys[idx], key) == std::strong_ordering::less) ++idx;
                InsertResult child_res = insert_recursive(node->children[idx], key);
                if (!child_res.new_right) return InsertResult{};
                // need to insert promoted key and pointer
                auto itk = lower_bound_in_keys(node->keys, child_res.promoted_key);
                size_t insert_pos = itk - node->keys.begin();
                node->keys.insert(node->keys.begin() + insert_pos, child_res.promoted_key);
                node->children.insert(node->children.begin() + insert_pos + 1, child_res.new_right);
                if ((int)node->keys.size() > max_keys_) {
                    // split internal node
                    size_t total = node->keys.size();
                    size_t mid = total / 2;
                    T promoted = node->keys[mid];
                    Node* new_internal = new Node(false);
                    // keys after mid are moved to new_internal (excluding promoted)
                    new_internal->keys.assign(node->keys.begin() + mid + 1, node->keys.end());
                    node->keys.erase(node->keys.begin() + mid, node->keys.end()); // remove mid and right-part
                    // children: move corresponding children
                    new_internal->children.assign(node->children.begin() + mid + 1, node->children.end());
                    node->children.erase(node->children.begin() + mid + 1, node->children.end());
                    InsertResult r;
                    r.new_right = new_internal;
                    r.promoted_key = promoted;
                    return r;
                }
                else {
                    return InsertResult{};
                }
            }
        }

        // Erase helper returns whether a deletion actually happened and handles rebalancing
        bool erase_recursive(Node* node, const T& key) {
            if (node->is_leaf) {
                auto it = lower_bound_in_keys(node->keys, key);
                if (it == node->keys.end() || comp_(*it, key) != std::strong_ordering::equal) {
                    return false; // not found
                }
                node->keys.erase(it);
                // Underflow handling will be done by parent via borrow/merge; but here we return true and let parent fix
                return true;
            }
            else {
                // find child
                size_t idx = 0;
                while (idx < node->keys.size() && comp_(node->keys[idx], key) == std::strong_ordering::less) ++idx;
                bool removed = erase_recursive(node->children[idx], key);
                if (!removed) return false;
                // After deletion, check child underflow
                Node* child = node->children[idx];
                if (child == root_) {
                    // shouldn't happen
                }
                if ((int)child->keys.size() < min_keys_) {
                    // try borrow from left sibling
                    Node* left = (idx > 0) ? node->children[idx - 1] : nullptr;
                    Node* right = (idx + 1 < node->children.size()) ? node->children[idx + 1] : nullptr;
                    if (left && (int)left->keys.size() > min_keys_) {
                        // borrow from left
                        if (child->is_leaf) {
                            child->keys.insert(child->keys.begin(), left->keys.back());
                            left->keys.pop_back();
                            node->keys[idx - 1] = child->keys.front();
                        }
                        else {
                            // for internal nodes, rotate from left
                            child->keys.insert(child->keys.begin(), node->keys[idx - 1]);
                            node->keys[idx - 1] = left->keys.back();
                            left->keys.pop_back();
                            child->children.insert(child->children.begin(), left->children.back());
                            left->children.pop_back();
                        }
                    }
                    else if (right && (int)right->keys.size() > min_keys_) {
                        // borrow from right
                        if (child->is_leaf) {
                            child->keys.push_back(right->keys.front());
                            right->keys.erase(right->keys.begin());
                            node->keys[idx] = right->keys.front();
                        }
                        else {
                            child->keys.push_back(node->keys[idx]);
                            node->keys[idx] = right->keys.front();
                            right->keys.erase(right->keys.begin());
                            child->children.push_back(right->children.front());
                            right->children.erase(right->children.begin());
                        }
                    }
                    else if (left) {
                        // merge child into left
                        if (child->is_leaf) {
                            left->keys.insert(left->keys.end(), child->keys.begin(), child->keys.end());
                            left->next = child->next;
                            delete child;
                            node->children.erase(node->children.begin() + idx);
                            node->keys.erase(node->keys.begin() + (idx - 1));
                        }
                        else {
                            // internal merge: left keys + separator + child keys
                            left->keys.push_back(node->keys[idx - 1]);
                            left->keys.insert(left->keys.end(), child->keys.begin(), child->keys.end());
                            left->children.insert(left->children.end(), child->children.begin(), child->children.end());
                            delete child;
                            node->children.erase(node->children.begin() + idx);
                            node->keys.erase(node->keys.begin() + (idx - 1));
                        }
                    }
                    else if (right) {
                        // merge right into child
                        if (child->is_leaf) {
                            child->keys.insert(child->keys.end(), right->keys.begin(), right->keys.end());
                            child->next = right->next;
                            delete right;
                            node->children.erase(node->children.begin() + (idx + 1));
                            node->keys.erase(node->keys.begin() + idx);
                        }
                        else {
                            child->keys.push_back(node->keys[idx]);
                            child->keys.insert(child->keys.end(), right->keys.begin(), right->keys.end());
                            child->children.insert(child->children.end(), right->children.begin(), right->children.end());
                            delete right;
                            node->children.erase(node->children.begin() + (idx + 1));
                            node->keys.erase(node->keys.begin() + idx);
                        }
                    }
                }
                return true;
            }
        }

        void delete_subtree(Node* node) {
            if (!node) return;
            if (!node->is_leaf) {
                for (auto c : node->children) delete_subtree(c);
                node->children.clear();
            }
            delete node;
        }
    };

}