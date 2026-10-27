#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <optional>
#include <functional>
#include <type_traits>
#include "external/json.hpp"

#include "core/types.hpp"
#include "core/trace.hpp"

namespace emergencygrid {

/**
 * @brief Node representation for AVLTree.
 *
 * Supports generic Key and Value types. For DSA Lab visualization, serialization
 * methods convert nodes to JSON snapshots.
 */
template <typename Key, typename Value>
struct AVLNode {
    Key key;
    Value value;
    int height{1};
    std::unique_ptr<AVLNode<Key, Value>> left{nullptr};
    std::unique_ptr<AVLNode<Key, Value>> right{nullptr};

    AVLNode(const Key& k, const Value& v)
        : key(k), value(v), height(1), left(nullptr), right(nullptr) {}

    AVLNode(Key&& k, Value&& v)
        : key(std::move(k)), value(std::move(v)), height(1), left(nullptr), right(nullptr) {}
};

/**
 * @brief Self-balancing AVL Tree (Adelson-Velsky and Landis).
 *
 * Guarantees O(log N) worst-case time complexity for search, insertion, and deletion.
 * Enforces the AVL balance invariant: for every node, |height(left) - height(right)| <= 1.
 * When the invariant is violated after an insertion or deletion, four rotation types
 * restore balance:
 *   - Left-Left (LL): Single Right Rotation
 *   - Right-Right (RR): Single Left Rotation
 *   - Left-Right (LR): Double Rotation (Left on child, Right on pivot)
 *   - Right-Left (RL): Double Rotation (Right on child, Left on pivot)
 *
 * Integrates with the EmergencyGrid TraceCollector system to emit granular trace events
 * for each comparison, height update, rotation, and rebalancing step.
 */
template <typename Key, typename Value>
class AVLTree {
public:
    AVLTree() = default;
    ~AVLTree() = default;

    // Disallow copy, allow move
    AVLTree(const AVLTree&) = delete;
    AVLTree& operator=(const AVLTree&) = delete;
    AVLTree(AVLTree&&) noexcept = default;
    AVLTree& operator=(AVLTree&&) noexcept = default;

    /**
     * @brief Inserts a record. Rejects duplicate keys according to project policy.
     * @return true if inserted, false if key already exists.
     */
    bool insert(const Key& key, const Value& value, TraceCollector* trace = nullptr) {
        bool inserted = false;
        insertInternal(root_, key, value, trace, inserted);
        if (inserted) {
            size_++;
            if (trace && trace->isEnabled()) {
                TraceEvent ev;
                ev.algorithm = "AVLTree";
                ev.operation = "insert_completed";
                ev.entityId = keyToString(key);
                ev.metadata = {
                    {"key", keyToString(key)},
                    {"treeSize", size_},
                    {"treeHeight", height()},
                    {"treeSnapshot", nodeToJson(root_.get())}
                };
                ev.explanation = "Completed insertion of node '" + keyToString(key) + "'.";
                trace->addEvent(std::move(ev));
            }
        }
        return inserted;
    }

    /**
     * @brief Searches for a record by key.
     * @return Pointer to value if found, nullptr otherwise.
     */
    Value* search(const Key& key, TraceCollector* trace = nullptr) {
        AVLNode<Key, Value>* curr = root_.get();
        while (curr) {
            if (trace && trace->isEnabled()) {
                TraceEvent ev;
                ev.algorithm = "AVLTree";
                ev.operation = "compare";
                ev.entityId = keyToString(key);
                ev.metadata = {
                    {"searchKey", keyToString(key)},
                    {"currentNodeKey", keyToString(curr->key)},
                    {"currentHeight", curr->height},
                    {"balanceFactor", balanceFactor(curr)},
                    {"treeSnapshot", nodeToJson(root_.get())}
                };
                ev.explanation = "Comparing search key '" + keyToString(key) +
                                 "' with node key '" + keyToString(curr->key) + "'.";
                trace->addEvent(std::move(ev));
            }

            if (key == curr->key) {
                if (trace && trace->isEnabled()) {
                    TraceEvent ev;
                    ev.algorithm = "AVLTree";
                    ev.operation = "search_found";
                    ev.entityId = keyToString(key);
                    ev.metadata = {
                        {"foundKey", keyToString(key)},
                        {"treeSnapshot", nodeToJson(root_.get())}
                    };
                    ev.explanation = "Found key '" + keyToString(key) + "' in AVL tree.";
                    trace->addEvent(std::move(ev));
                }
                return &curr->value;
            } else if (key < curr->key) {
                curr = curr->left.get();
            } else {
                curr = curr->right.get();
            }
        }

        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "AVLTree";
            ev.operation = "search_not_found";
            ev.entityId = keyToString(key);
            ev.metadata = {
                {"missingKey", keyToString(key)},
                {"treeSnapshot", nodeToJson(root_.get())}
            };
            ev.explanation = "Key '" + keyToString(key) + "' not found in AVL tree.";
            trace->addEvent(std::move(ev));
        }
        return nullptr;
    }

    const Value* search(const Key& key, TraceCollector* trace = nullptr) const {
        return const_cast<AVLTree*>(this)->search(key, trace);
    }

    /**
     * @brief Checks whether a key exists in the tree.
     */
    bool contains(const Key& key, TraceCollector* trace = nullptr) const {
        return search(key, trace) != nullptr;
    }

    /**
     * @brief Deletes a record by key and rebalances the tree.
     * @return true if deleted, false if key was not found.
     */
    bool remove(const Key& key, TraceCollector* trace = nullptr) {
        bool deleted = false;
        removeInternal(root_, key, trace, deleted);
        if (deleted) {
            size_--;
            if (trace && trace->isEnabled()) {
                TraceEvent ev;
                ev.algorithm = "AVLTree";
                ev.operation = "remove_completed";
                ev.entityId = keyToString(key);
                ev.metadata = {
                    {"key", keyToString(key)},
                    {"treeSize", size_},
                    {"treeHeight", height()},
                    {"treeSnapshot", nodeToJson(root_.get())}
                };
                ev.explanation = "Completed removal of node '" + keyToString(key) + "'.";
                trace->addEvent(std::move(ev));
            }
        }
        return deleted;
    }

    /**
     * @brief Updates an existing key's value.
     * @return true if updated, false if key was not found.
     */
    bool update(const Key& key, const Value& value, TraceCollector* trace = nullptr) {
        Value* existing = search(key, trace);
        if (existing) {
            *existing = value;
            if (trace && trace->isEnabled()) {
                TraceEvent ev;
                ev.algorithm = "AVLTree";
                ev.operation = "update";
                ev.entityId = keyToString(key);
                ev.metadata = {
                    {"updatedKey", keyToString(key)},
                    {"treeSnapshot", nodeToJson(root_.get())}
                };
                ev.explanation = "Updated value for key '" + keyToString(key) + "'.";
                trace->addEvent(std::move(ev));
            }
            return true;
        }
        return false;
    }

    /**
     * @brief Performs an inorder traversal returning values in ascending key order.
     */
    std::vector<Value> inorderTraversal(TraceCollector* trace = nullptr) const {
        std::vector<Value> result;
        result.reserve(size_);
        inorderInternal(root_.get(), result, trace);
        return result;
    }

    /**
     * @brief Performs an inorder traversal returning keys in ascending order.
     */
    std::vector<Key> inorderKeys(TraceCollector* trace = nullptr) const {
        std::vector<Key> result;
        result.reserve(size_);
        inorderKeysInternal(root_.get(), result, trace);
        return result;
    }

    // Queries
    size_t size() const noexcept { return size_; }
    bool isEmpty() const noexcept { return size_ == 0; }
    int height() const noexcept { return nodeHeight(root_.get()); }

    void clear() noexcept {
        root_.reset();
        size_ = 0;
    }

    // Direct access for testing
    static int nodeHeight(const AVLNode<Key, Value>* node) noexcept {
        return node ? node->height : 0;
    }

    static int balanceFactor(const AVLNode<Key, Value>* node) noexcept {
        return node ? (nodeHeight(node->left.get()) - nodeHeight(node->right.get())) : 0;
    }

    /**
     * @brief Generates a structured JSON hierarchy snapshot for DSA Lab visualization.
     */
    nlohmann::json toJsonSnapshot() const {
        return {
            {"size", size_},
            {"height", height()},
            {"isEmpty", isEmpty()},
            {"root", nodeToJson(root_.get())}
        };
    }

private:
    std::unique_ptr<AVLNode<Key, Value>> root_{nullptr};
    size_t size_{0};

    static std::string keyToString(const Key& k) {
        if constexpr (std::is_same_v<Key, std::string>) {
            return k;
        } else {
            return std::to_string(k);
        }
    }

    nlohmann::json nodeToJson(const AVLNode<Key, Value>* node) const {
        if (!node) return nullptr;
        nlohmann::json j;
        j["key"] = keyToString(node->key);
        j["height"] = node->height;
        j["balanceFactor"] = balanceFactor(node);

        if constexpr (std::is_same_v<Value, Incident>) {
            j["incidentId"] = node->value.id;
            j["type"] = incidentTypeToString(node->value.type);
            j["severity"] = static_cast<int>(node->value.severity);
            j["status"] = incidentStatusToString(node->value.status);
            j["locationId"] = node->value.locationId;
        }

        if (node->left) {
            j["leftChild"] = keyToString(node->left->key);
        } else {
            j["leftChild"] = nullptr;
        }

        if (node->right) {
            j["rightChild"] = keyToString(node->right->key);
        } else {
            j["rightChild"] = nullptr;
        }
        j["left"] = nodeToJson(node->left.get());
        j["right"] = nodeToJson(node->right.get());
        return j;
    }

    void updateHeight(AVLNode<Key, Value>* node, TraceCollector* trace) {
        if (!node) return;
        int oldH = node->height;
        node->height = 1 + std::max(nodeHeight(node->left.get()), nodeHeight(node->right.get()));

        if (trace && trace->isEnabled() && oldH != node->height) {
            TraceEvent ev;
            ev.algorithm = "AVLTree";
            ev.operation = "height_update";
            ev.entityId = keyToString(node->key);
            ev.previousValue = static_cast<double>(oldH);
            ev.newValue = static_cast<double>(node->height);
            ev.metadata = {
                {"nodeKey", keyToString(node->key)},
                {"oldHeight", oldH},
                {"newHeight", node->height},
                {"balanceFactor", balanceFactor(node)},
                {"treeSnapshot", nodeToJson(root_.get())}
            };
            ev.explanation = "Updated height of node '" + keyToString(node->key) +
                             "' from " + std::to_string(oldH) + " to " + std::to_string(node->height) + ".";
            trace->addEvent(std::move(ev));
        }
    }

    // -------------------------------------------------------------
    // AVL ROTATIONS
    // -------------------------------------------------------------

    /**
     * @brief Right Rotation (LL Case).
     *
     *       y (root)                  x
     *      / \                       / \
     *     x   T3       ===>         T1  y
     *    / \                           / \
     *   T1  T2                        T2  T3
     */
    void rotateLL(
        std::unique_ptr<AVLNode<Key, Value>>& y,
        TraceCollector* trace
    ) {
        std::string yKey = keyToString(y->key);
        std::unique_ptr<AVLNode<Key, Value>> x = std::move(y->left);
        std::string xKey = keyToString(x->key);

        y->left = std::move(x->right);
        updateHeight(y.get(), trace);

        x->right = std::move(y);
        updateHeight(x.get(), trace);

        y = std::move(x);

        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "AVLTree";
            ev.operation = "rotate_ll";
            ev.entityId = yKey;
            ev.from = yKey;
            ev.to = xKey;
            ev.metadata = {
                {"rotationType", "LL"},
                {"pivotKey", yKey},
                {"promotedKey", xKey},
                {"newPivotHeight", y->height},
                {"newBalanceFactor", balanceFactor(y.get())},
                {"treeSnapshot", nodeToJson(root_.get())}
            };
            ev.explanation = "Executed LL (Right) rotation on pivot '" + yKey +
                             "'; promoted left child '" + xKey + "' to subtree root.";
            trace->addEvent(std::move(ev));
        }
    }

    /**
     * @brief Left Rotation (RR Case).
     *
     *     x (root)                    y
     *    / \                         / \
     *   T1  y          ===>         x   T3
     *      / \                     / \
     *     T2  T3                  T1  T2
     */
    void rotateRR(
        std::unique_ptr<AVLNode<Key, Value>>& x,
        TraceCollector* trace
    ) {
        std::string xKey = keyToString(x->key);
        std::unique_ptr<AVLNode<Key, Value>> y = std::move(x->right);
        std::string yKey = keyToString(y->key);

        x->right = std::move(y->left);
        updateHeight(x.get(), trace);

        y->left = std::move(x);
        updateHeight(y.get(), trace);

        x = std::move(y);

        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "AVLTree";
            ev.operation = "rotate_rr";
            ev.entityId = xKey;
            ev.from = xKey;
            ev.to = yKey;
            ev.metadata = {
                {"rotationType", "RR"},
                {"pivotKey", xKey},
                {"promotedKey", yKey},
                {"newPivotHeight", x->height},
                {"newBalanceFactor", balanceFactor(x.get())},
                {"treeSnapshot", nodeToJson(root_.get())}
            };
            ev.explanation = "Executed RR (Left) rotation on pivot '" + xKey +
                             "'; promoted right child '" + yKey + "' to subtree root.";
            trace->addEvent(std::move(ev));
        }
    }

    /**
     * @brief Left-Right Rotation (LR Case).
     * First left rotates left child (RR), then right rotates root (LL).
     */
    void rotateLR(
        std::unique_ptr<AVLNode<Key, Value>>& node,
        TraceCollector* trace
    ) {
        std::string pivotKey = keyToString(node->key);
        std::string childKey = keyToString(node->left->key);

        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "AVLTree";
            ev.operation = "rotate_lr_start";
            ev.entityId = pivotKey;
            ev.metadata = {
                {"pivotKey", pivotKey},
                {"childKey", childKey},
                {"treeSnapshot", nodeToJson(root_.get())}
            };
            ev.explanation = "Starting double LR rotation on pivot '" + pivotKey + "': left child is right-heavy.";
            trace->addEvent(std::move(ev));
        }

        rotateRR(node->left, trace);

        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "AVLTree";
            ev.operation = "rotate_lr_intermediate";
            ev.entityId = pivotKey;
            ev.metadata = {
                {"pivotKey", pivotKey},
                {"childKey", childKey},
                {"treeSnapshot", nodeToJson(root_.get())}
            };
            ev.explanation = "Step 1 of LR rotation complete: left child rotated into straight LL configuration.";
            trace->addEvent(std::move(ev));
        }

        rotateLL(node, trace);
    }

    /**
     * @brief Right-Left Rotation (RL Case).
     * First right rotates right child (LL), then left rotates root (RR).
     */
    void rotateRL(
        std::unique_ptr<AVLNode<Key, Value>>& node,
        TraceCollector* trace
    ) {
        std::string pivotKey = keyToString(node->key);
        std::string childKey = keyToString(node->right->key);

        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "AVLTree";
            ev.operation = "rotate_rl_start";
            ev.entityId = pivotKey;
            ev.metadata = {
                {"pivotKey", pivotKey},
                {"childKey", childKey},
                {"treeSnapshot", nodeToJson(root_.get())}
            };
            ev.explanation = "Starting double RL rotation on pivot '" + pivotKey + "': right child is left-heavy.";
            trace->addEvent(std::move(ev));
        }

        rotateLL(node->right, trace);

        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "AVLTree";
            ev.operation = "rotate_rl_intermediate";
            ev.entityId = pivotKey;
            ev.metadata = {
                {"pivotKey", pivotKey},
                {"childKey", childKey},
                {"treeSnapshot", nodeToJson(root_.get())}
            };
            ev.explanation = "Step 1 of RL rotation complete: right child rotated into straight RR configuration.";
            trace->addEvent(std::move(ev));
        }

        rotateRR(node, trace);
    }

    /**
     * @brief Rebalances an AVL subtree at `node` if the AVL invariant (|bf| <= 1) is violated.
     */
    void rebalance(
        std::unique_ptr<AVLNode<Key, Value>>& node,
        TraceCollector* trace
    ) {
        if (!node) return;

        updateHeight(node.get(), trace);
        int bf = balanceFactor(node.get());

        if (trace && trace->isEnabled() && (bf < -1 || bf > 1)) {
            TraceEvent ev;
            ev.algorithm = "AVLTree";
            ev.operation = "balance_factor";
            ev.entityId = keyToString(node->key);
            ev.metadata = {
                {"nodeKey", keyToString(node->key)},
                {"balanceFactor", bf},
                {"unbalanced", true},
                {"treeSnapshot", nodeToJson(root_.get())}
            };
            ev.explanation = "Node '" + keyToString(node->key) + "' has balance factor " +
                             std::to_string(bf) + " (AVL invariant violated, rebalancing required).";
            trace->addEvent(std::move(ev));
        }

        // Left heavy
        if (bf > 1) {
            if (balanceFactor(node->left.get()) >= 0) {
                // LL Case
                rotateLL(node, trace);
            } else {
                // LR Case
                rotateLR(node, trace);
            }
        } else if (bf < -1) {
            // Right heavy
            if (balanceFactor(node->right.get()) <= 0) {
                // RR Case
                rotateRR(node, trace);
            } else {
                // RL Case
                rotateRL(node, trace);
            }
        }
    }

    void insertInternal(
        std::unique_ptr<AVLNode<Key, Value>>& node,
        const Key& key,
        const Value& value,
        TraceCollector* trace,
        bool& inserted
    ) {
        if (!node) {
            inserted = true;
            node = std::make_unique<AVLNode<Key, Value>>(key, value);
            if (trace && trace->isEnabled()) {
                TraceEvent ev;
                ev.algorithm = "AVLTree";
                ev.operation = "insert";
                ev.entityId = keyToString(key);
                ev.metadata = {
                    {"key", keyToString(key)},
                    {"height", 1},
                    {"balanceFactor", 0},
                    {"treeSnapshot", nodeToJson(root_.get())}
                };
                ev.explanation = "Inserted new node with key '" + keyToString(key) + "'.";
                trace->addEvent(std::move(ev));
            }
            return;
        }

        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "AVLTree";
            ev.operation = "compare";
            ev.entityId = keyToString(key);
            ev.metadata = {
                {"searchKey", keyToString(key)},
                {"currentNodeKey", keyToString(node->key)},
                {"currentHeight", node->height},
                {"balanceFactor", balanceFactor(node.get())},
                {"treeSnapshot", nodeToJson(root_.get())}
            };
            ev.explanation = "Comparing key '" + keyToString(key) +
                             "' with node key '" + keyToString(node->key) + "'.";
            trace->addEvent(std::move(ev));
        }

        if (key < node->key) {
            insertInternal(node->left, key, value, trace, inserted);
        } else if (key > node->key) {
            insertInternal(node->right, key, value, trace, inserted);
        } else {
            // Duplicate key rejected
            inserted = false;
            if (trace && trace->isEnabled()) {
                TraceEvent ev;
                ev.algorithm = "AVLTree";
                ev.operation = "duplicate_rejected";
                ev.entityId = keyToString(key);
                ev.metadata = {
                    {"rejectedKey", keyToString(key)},
                    {"treeSnapshot", nodeToJson(root_.get())}
                };
                ev.explanation = "Rejected insertion of duplicate key '" + keyToString(key) + "'.";
                trace->addEvent(std::move(ev));
            }
            return;
        }

        rebalance(node, trace);
    }

    static AVLNode<Key, Value>* findMin(AVLNode<Key, Value>* node) noexcept {
        while (node && node->left) {
            node = node->left.get();
        }
        return node;
    }

    void removeInternal(
        std::unique_ptr<AVLNode<Key, Value>>& node,
        const Key& key,
        TraceCollector* trace,
        bool& deleted
    ) {
        if (!node) {
            deleted = false;
            return;
        }

        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "AVLTree";
            ev.operation = "compare";
            ev.entityId = keyToString(key);
            ev.metadata = {
                {"targetKey", keyToString(key)},
                {"currentNodeKey", keyToString(node->key)},
                {"currentHeight", node->height},
                {"balanceFactor", balanceFactor(node.get())},
                {"treeSnapshot", nodeToJson(root_.get())}
            };
            ev.explanation = "Comparing key '" + keyToString(key) +
                             "' with node key '" + keyToString(node->key) + "'.";
            trace->addEvent(std::move(ev));
        }

        if (key < node->key) {
            removeInternal(node->left, key, trace, deleted);
        } else if (key > node->key) {
            removeInternal(node->right, key, trace, deleted);
        } else {
            // Node found
            deleted = true;

            if (trace && trace->isEnabled()) {
                TraceEvent ev;
                ev.algorithm = "AVLTree";
                ev.operation = "delete";
                ev.entityId = keyToString(key);
                ev.metadata = {
                    {"deletedKey", keyToString(key)},
                    {"hadLeftChild", node->left != nullptr},
                    {"hadRightChild", node->right != nullptr},
                    {"treeSnapshot", nodeToJson(root_.get())}
                };
                ev.explanation = "Deleted node with key '" + keyToString(key) + "'.";
                trace->addEvent(std::move(ev));
            }

            // Case 1: Leaf node (no children)
            if (!node->left && !node->right) {
                node.reset();
                return;
            }
            // Case 2: One child (right only)
            if (!node->left) {
                node = std::move(node->right);
                return;
            }
            // Case 2: One child (left only)
            if (!node->right) {
                node = std::move(node->left);
                return;
            }

            // Case 3: Two children -> find inorder successor (smallest in right subtree)
            AVLNode<Key, Value>* successor = findMin(node->right.get());
            Key successorKey = successor->key;
            Value successorVal = successor->value;

            node->key = successorKey;
            node->value = successorVal;

            bool subDeleted = false;
            removeInternal(node->right, successorKey, trace, subDeleted);
        }

        rebalance(node, trace);
    }

    void inorderInternal(
        const AVLNode<Key, Value>* node,
        std::vector<Value>& result,
        TraceCollector* trace
    ) const {
        if (!node) return;
        inorderInternal(node->left.get(), result, trace);

        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "AVLTree";
            ev.operation = "inorder_visit";
            ev.entityId = keyToString(node->key);
            ev.metadata = {
                {"visitedKey", keyToString(node->key)},
                {"height", node->height},
                {"balanceFactor", balanceFactor(node)},
                {"treeSnapshot", nodeToJson(root_.get())}
            };
            ev.explanation = "Inorder visit at node '" + keyToString(node->key) + "'.";
            trace->addEvent(std::move(ev));
        }
        result.push_back(node->value);

        inorderInternal(node->right.get(), result, trace);
    }

    void inorderKeysInternal(
        const AVLNode<Key, Value>* node,
        std::vector<Key>& result,
        TraceCollector* trace
    ) const {
        if (!node) return;
        inorderKeysInternal(node->left.get(), result, trace);

        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "AVLTree";
            ev.operation = "inorder_visit";
            ev.entityId = keyToString(node->key);
            ev.metadata = {
                {"visitedKey", keyToString(node->key)},
                {"height", node->height},
                {"balanceFactor", balanceFactor(node)},
                {"treeSnapshot", nodeToJson(root_.get())}
            };
            ev.explanation = "Inorder visit at node '" + keyToString(node->key) + "'.";
            trace->addEvent(std::move(ev));
        }
        result.push_back(node->key);

        inorderKeysInternal(node->right.get(), result, trace);
    }
};

using IncidentAVLTree = AVLTree<std::string, Incident>;

} // namespace emergencygrid
