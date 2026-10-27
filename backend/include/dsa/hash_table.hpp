#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <functional>
#include "core/types.hpp"
#include "core/trace.hpp"
#include "json.hpp"

namespace emergencygrid {

/**
 * @brief Key-Value entry within a hash bucket chain.
 */
template <typename Key = std::string, typename Value = Incident>
struct HashEntry {
    Key key;
    Value value;

    HashEntry(Key k, Value v)
        : key(std::move(k)), value(std::move(v)) {}
};

/**
 * @brief Custom Hash Table implemented in C++20 using Separate Chaining.
 *
 * Implements genuine educational hash table mechanics without std::unordered_map.
 * Features:
 * - Deterministic FNV-1a 64-bit hashing algorithm.
 * - Dynamic capacity scaling with automatic rehashing upon exceeding the load-factor threshold.
 * - Collision handling via per-bucket separate chaining.
 * - Fine-grained trace event emission for DSA Lab inspector.
 * - Safe statistics export (bucket depths, collision count, load factor).
 */
template <typename Key = std::string, typename Value = Incident>
class HashTable {
public:
    static constexpr size_t DEFAULT_INITIAL_BUCKETS = 16;
    static constexpr double DEFAULT_MAX_LOAD_FACTOR = 0.75;

    explicit HashTable(
        size_t initialBuckets = DEFAULT_INITIAL_BUCKETS,
        double maxLoadFactor = DEFAULT_MAX_LOAD_FACTOR
    ) : bucketCount_(std::max<size_t>(initialBuckets, 4)),
        maxLoadFactor_(maxLoadFactor),
        buckets_(bucketCount_) {}

    ~HashTable() = default;

    HashTable(const HashTable&) = default;
    HashTable& operator=(const HashTable&) = default;
    HashTable(HashTable&&) noexcept = default;
    HashTable& operator=(HashTable&&) noexcept = default;

    /**
     * @brief Computes deterministic 64-bit FNV-1a hash of a key.
     */
    static uint64_t hashKey(const Key& key) noexcept {
        const std::string s = keyToString(key);
        uint64_t hash = 14695981039346656037ULL; // FNV-1a 64-bit offset basis
        for (char c : s) {
            hash ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
            hash *= 1099511628211ULL;            // FNV-1a prime
        }
        return hash;
    }

    /**
     * @brief Inserts a new key-value pair.
     * Rejects duplicate keys according to project policy.
     * @return true if inserted, false if key already exists.
     */
    bool insert(const Key& key, const Value& value, TraceCollector* trace = nullptr) {
        // Check load factor and rehash if needed before insertion
        if (static_cast<double>(size_ + 1) > static_cast<double>(bucketCount_) * maxLoadFactor_) {
            rehash(bucketCount_ * 2, trace);
        }

        uint64_t rawHash = hashKey(key);
        size_t bIdx = getBucketIndex(rawHash);

        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "HashTable";
            ev.operation = "hash_key";
            ev.entityId = keyToString(key);
            ev.metadata = {
                {"key", keyToString(key)},
                {"rawHash", std::to_string(rawHash)},
                {"bucketIndex", bIdx},
                {"bucketCount", bucketCount_}
            };
            ev.explanation = "Hashed key '" + keyToString(key) + "' -> bucket index " +
                             std::to_string(bIdx) + " (table capacity: " + std::to_string(bucketCount_) + ").";
            trace->addEvent(std::move(ev));
        }

        auto& chain = buckets_[bIdx];

        // Traverse bucket chain to verify uniqueness
        for (size_t i = 0; i < chain.size(); ++i) {
            if (trace && trace->isEnabled()) {
                TraceEvent ev;
                ev.algorithm = "HashTable";
                ev.operation = "bucket_traversal";
                ev.entityId = keyToString(key);
                ev.metadata = {
                    {"bucketIndex", bIdx},
                    {"chainIndex", i},
                    {"examinedKey", keyToString(chain[i].key)},
                    {"targetKey", keyToString(key)}
                };
                ev.explanation = "Inspecting entry at chain position " + std::to_string(i) +
                                 " in bucket " + std::to_string(bIdx) + ".";
                trace->addEvent(std::move(ev));
            }

            if (chain[i].key == key) {
                // Key already exists -> reject duplicate
                if (trace && trace->isEnabled()) {
                    TraceEvent ev;
                    ev.algorithm = "HashTable";
                    ev.operation = "duplicate_rejected";
                    ev.entityId = keyToString(key);
                    ev.metadata = {
                        {"bucketIndex", bIdx},
                        {"rejectedKey", keyToString(key)}
                    };
                    ev.explanation = "Rejected insertion of duplicate key '" + keyToString(key) + "'.";
                    trace->addEvent(std::move(ev));
                }
                return false;
            }
        }

        // Detect collision if bucket is non-empty
        if (!chain.empty() && trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "HashTable";
            ev.operation = "collision_detected";
            ev.entityId = keyToString(key);
            ev.metadata = {
                {"bucketIndex", bIdx},
                {"existingChainLength", chain.size()},
                {"newKey", keyToString(key)}
            };
            ev.explanation = "Collision in bucket " + std::to_string(bIdx) +
                             " (already contains " + std::to_string(chain.size()) + " entries). Chaining entry.";
            trace->addEvent(std::move(ev));
        }

        // Append to bucket chain
        chain.emplace_back(key, value);
        size_++;

        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "HashTable";
            ev.operation = "entry_inserted";
            ev.entityId = keyToString(key);
            ev.metadata = {
                {"key", keyToString(key)},
                {"bucketIndex", bIdx},
                {"chainLength", chain.size()},
                {"totalEntries", size_},
                {"loadFactor", loadFactor()}
            };
            ev.explanation = "Inserted entry '" + keyToString(key) + "' into bucket " +
                             std::to_string(bIdx) + " (new chain length: " + std::to_string(chain.size()) + ").";
            trace->addEvent(std::move(ev));
        }

        return true;
    }

    /**
     * @brief Searches for an entry by key.
     * @return Pointer to value if found, nullptr otherwise.
     */
    Value* search(const Key& key, TraceCollector* trace = nullptr) {
        uint64_t rawHash = hashKey(key);
        size_t bIdx = getBucketIndex(rawHash);

        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "HashTable";
            ev.operation = "hash_key";
            ev.entityId = keyToString(key);
            ev.metadata = {
                {"key", keyToString(key)},
                {"bucketIndex", bIdx}
            };
            ev.explanation = "Looking up key '" + keyToString(key) + "' in bucket " + std::to_string(bIdx) + ".";
            trace->addEvent(std::move(ev));
        }

        auto& chain = buckets_[bIdx];
        for (size_t i = 0; i < chain.size(); ++i) {
            if (trace && trace->isEnabled()) {
                TraceEvent ev;
                ev.algorithm = "HashTable";
                ev.operation = "bucket_traversal";
                ev.entityId = keyToString(key);
                ev.metadata = {
                    {"bucketIndex", bIdx},
                    {"chainIndex", i},
                    {"examinedKey", keyToString(chain[i].key)},
                    {"targetKey", keyToString(key)}
                };
                ev.explanation = "Examining chain entry " + std::to_string(i) + " in bucket " + std::to_string(bIdx) + ".";
                trace->addEvent(std::move(ev));
            }

            if (chain[i].key == key) {
                if (trace && trace->isEnabled()) {
                    TraceEvent ev;
                    ev.algorithm = "HashTable";
                    ev.operation = "entry_found";
                    ev.entityId = keyToString(key);
                    ev.metadata = {
                        {"key", keyToString(key)},
                        {"bucketIndex", bIdx},
                        {"chainPosition", i}
                    };
                    ev.explanation = "Found key '" + keyToString(key) + "' in bucket " +
                                     std::to_string(bIdx) + " after " + std::to_string(i + 1) + " probes.";
                    trace->addEvent(std::move(ev));
                }
                return &chain[i].value;
            }
        }

        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "HashTable";
            ev.operation = "entry_not_found";
            ev.entityId = keyToString(key);
            ev.metadata = {
                {"key", keyToString(key)},
                {"bucketIndex", bIdx},
                {"probedEntries", chain.size()}
            };
            ev.explanation = "Key '" + keyToString(key) + "' not found in bucket " + std::to_string(bIdx) + ".";
            trace->addEvent(std::move(ev));
        }

        return nullptr;
    }

    const Value* search(const Key& key, TraceCollector* trace = nullptr) const {
        return const_cast<HashTable*>(this)->search(key, trace);
    }

    /**
     * @brief Checks whether a key exists in the table.
     */
    bool contains(const Key& key, TraceCollector* trace = nullptr) const {
        return search(key, trace) != nullptr;
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
                ev.algorithm = "HashTable";
                ev.operation = "entry_updated";
                ev.entityId = keyToString(key);
                ev.metadata = {{"updatedKey", keyToString(key)}};
                ev.explanation = "Updated value for key '" + keyToString(key) + "'.";
                trace->addEvent(std::move(ev));
            }
            return true;
        }
        return false;
    }

    /**
     * @brief Deletes an entry by key.
     * @return true if deleted, false if key was not found.
     */
    bool remove(const Key& key, TraceCollector* trace = nullptr) {
        uint64_t rawHash = hashKey(key);
        size_t bIdx = getBucketIndex(rawHash);

        auto& chain = buckets_[bIdx];
        for (auto it = chain.begin(); it != chain.end(); ++it) {
            if (it->key == key) {
                chain.erase(it);
                size_--;

                if (trace && trace->isEnabled()) {
                    TraceEvent ev;
                    ev.algorithm = "HashTable";
                    ev.operation = "entry_deleted";
                    ev.entityId = keyToString(key);
                    ev.metadata = {
                        {"key", keyToString(key)},
                        {"bucketIndex", bIdx},
                        {"remainingBucketLength", chain.size()},
                        {"totalEntries", size_}
                    };
                    ev.explanation = "Deleted entry with key '" + keyToString(key) +
                                     "' from bucket " + std::to_string(bIdx) + ".";
                    trace->addEvent(std::move(ev));
                }
                return true;
            }
        }

        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "HashTable";
            ev.operation = "entry_not_found";
            ev.entityId = keyToString(key);
            ev.metadata = {{"key", keyToString(key)}, {"bucketIndex", bIdx}};
            ev.explanation = "Cannot delete: key '" + keyToString(key) + "' not found in table.";
            trace->addEvent(std::move(ev));
        }

        return false;
    }

    /**
     * @brief Rehashes all elements into a new bucket array of size `newBucketCount`.
     */
    void rehash(size_t newBucketCount, TraceCollector* trace = nullptr) {
        if (newBucketCount < size_) {
            newBucketCount = size_ * 2;
        }
        if (newBucketCount < 4) {
            newBucketCount = 4;
        }

        size_t oldBucketCount = bucketCount_;
        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "HashTable";
            ev.operation = "rehash_started";
            ev.metadata = {
                {"oldBucketCount", oldBucketCount},
                {"newBucketCount", newBucketCount},
                {"currentSize", size_},
                {"currentLoadFactor", loadFactor()}
            };
            ev.explanation = "Rehashing table from " + std::to_string(oldBucketCount) +
                             " to " + std::to_string(newBucketCount) + " buckets.";
            trace->addEvent(std::move(ev));
        }

        std::vector<std::vector<HashEntry<Key, Value>>> newBuckets(newBucketCount);

        for (auto& oldChain : buckets_) {
            for (auto& entry : oldChain) {
                uint64_t rawHash = hashKey(entry.key);
                size_t newIdx = rawHash % newBucketCount;
                newBuckets[newIdx].push_back(std::move(entry));
            }
        }

        buckets_ = std::move(newBuckets);
        bucketCount_ = newBucketCount;

        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "HashTable";
            ev.operation = "rehash_completed";
            ev.metadata = {
                {"newBucketCount", bucketCount_},
                {"totalEntries", size_},
                {"newLoadFactor", loadFactor()},
                {"maxBucketDepth", maxBucketDepth()},
                {"collisionCount", collisionCount()}
            };
            ev.explanation = "Rehashing complete. New capacity: " + std::to_string(bucketCount_) +
                             " buckets (load factor: " + std::to_string(loadFactor()) + ").";
            trace->addEvent(std::move(ev));
        }
    }

    /**
     * @brief Clears all entries from the table.
     */
    void clear() noexcept {
        for (auto& chain : buckets_) {
            chain.clear();
        }
        size_ = 0;
    }

    // Queries & Metrics
    size_t size() const noexcept { return size_; }
    bool isEmpty() const noexcept { return size_ == 0; }
    size_t bucketCount() const noexcept { return bucketCount_; }
    double loadFactor() const noexcept {
        return (bucketCount_ == 0) ? 0.0 : static_cast<double>(size_) / static_cast<double>(bucketCount_);
    }
    double maxLoadFactor() const noexcept { return maxLoadFactor_; }

    size_t occupiedBucketCount() const noexcept {
        size_t count = 0;
        for (const auto& chain : buckets_) {
            if (!chain.empty()) count++;
        }
        return count;
    }

    size_t maxBucketDepth() const noexcept {
        size_t maxDepth = 0;
        for (const auto& chain : buckets_) {
            maxDepth = std::max(maxDepth, chain.size());
        }
        return maxDepth;
    }

    size_t collisionCount() const noexcept {
        size_t collisions = 0;
        for (const auto& chain : buckets_) {
            if (chain.size() > 1) {
                collisions += (chain.size() - 1);
            }
        }
        return collisions;
    }

    /**
     * @brief Returns a list of all stored values.
     */
    std::vector<Value> values() const {
        std::vector<Value> res;
        res.reserve(size_);
        for (const auto& chain : buckets_) {
            for (const auto& entry : chain) {
                res.push_back(entry.value);
            }
        }
        return res;
    }

    /**
     * @brief Returns safe statistics and occupancy distribution for DSA Lab.
     */
    nlohmann::json getStatsJson() const {
        nlohmann::json bucketsArr = nlohmann::json::array();
        for (size_t i = 0; i < bucketCount_; ++i) {
            nlohmann::json bInfo;
            bInfo["bucketIndex"] = i;
            bInfo["entryCount"] = buckets_[i].size();

            nlohmann::json keysArr = nlohmann::json::array();
            for (const auto& entry : buckets_[i]) {
                keysArr.push_back(keyToString(entry.key));
            }
            bInfo["keys"] = keysArr;
            bucketsArr.push_back(bInfo);
        }

        return {
            {"size", size_},
            {"bucketCount", bucketCount_},
            {"occupiedBuckets", occupiedBucketCount()},
            {"loadFactor", loadFactor()},
            {"maxLoadFactor", maxLoadFactor_},
            {"maxBucketDepth", maxBucketDepth()},
            {"collisionCount", collisionCount()},
            {"buckets", bucketsArr}
        };
    }

private:
    size_t bucketCount_{DEFAULT_INITIAL_BUCKETS};
    double maxLoadFactor_{DEFAULT_MAX_LOAD_FACTOR};
    size_t size_{0};
    std::vector<std::vector<HashEntry<Key, Value>>> buckets_;

    size_t getBucketIndex(uint64_t rawHash) const noexcept {
        return static_cast<size_t>(rawHash % bucketCount_);
    }

    static std::string keyToString(const Key& k) {
        if constexpr (std::is_same_v<Key, std::string>) {
            return k;
        } else {
            return std::to_string(k);
        }
    }
};

using IncidentHashTable = HashTable<std::string, Incident>;

} // namespace emergencygrid
