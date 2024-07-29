//
// Created by darion.yaphet on 2024/7/25.
//

#include <rocksdb/db.h>
#include <rocksdb/utilities/stackable_db.h>
#include <unordered_map>
#include <list>
#include <string>
#include <iostream>
#include <memory>

using namespace rocksdb;

class CachedDB : public StackableDB {
private:
    static const size_t CACHE_CAPACITY = 1024;

    struct CacheEntry {
        std::string key;
        std::string value;
        std::string timestamp;
    };

    std::unordered_map<std::string, std::list<CacheEntry>::iterator> cache_map;
    std::list<CacheEntry> cache_list;

    void add_to_cache(const std::string &key, const std::string &value, const std::string &timestamp) {
        if (cache_map.find(key) != cache_map.end()) {
            // Key exists, update its value and move it to the front
            auto it = cache_map[key];
            cache_list.erase(it);
            cache_list.push_front({key, value, timestamp});
            cache_map[key] = cache_list.begin();
        } else {
            // New key
            if (cache_map.size() >= CACHE_CAPACITY) {
                // Cache is full, remove the least recently used item
                auto last = cache_list.back();
                cache_map.erase(last.key);
                cache_list.pop_back();
            }
            cache_list.push_front({key, value, timestamp});
            cache_map[key] = cache_list.begin();
        }
    }

public:
    explicit CachedDB(DB *db) : StackableDB(db) {}

    Status Get(const ReadOptions &options, ColumnFamilyHandle *column_family,
               const Slice &key, PinnableSlice *value,
               std::string *timestamp) override {
        std::string string_key = key.ToString();
        auto it = cache_map.find(string_key);
        if (it != cache_map.end()) {
            // Cache hit
            value->PinSelf(it->second->value);
            if (timestamp) {
                *timestamp = it->second->timestamp;
            }
            return Status::OK();
        }
        // Cache miss, get from underlying DB
        Status s = StackableDB::Get(options, column_family, key, value, timestamp);
        if (s.ok()) {
            add_to_cache(string_key, value->ToString(), timestamp ? *timestamp : "");
        }
        return s;
    }

    Status Put(const WriteOptions &options, ColumnFamilyHandle *column_family,
               const Slice &key, const Slice &value) override {
        Status s = StackableDB::Put(options, column_family, key, value);
        if (s.ok()) {
            add_to_cache(key.ToString(), value.ToString(), "");
        }
        return s;
    }

    Status Delete(const WriteOptions &options, ColumnFamilyHandle *column_family,
                  const Slice &key) override {
        Status s = StackableDB::Delete(options, column_family, key);
        if (s.ok()) {
            std::string string_key = key.ToString();
            auto it = cache_map.find(string_key);
            if (it != cache_map.end()) {
                cache_list.erase(it->second);
                cache_map.erase(it);
            }
        }
        return s;
    }
};

int main() {
    // Open a RocksDB database
    DB *base_db;
    Options options;
    options.create_if_missing = true;
    Status s = DB::Open(options, "/tmp/testdb", &base_db);
    if (!s.ok()) {
        std::cerr << "Unable to open database: " << s.ToString() << std::endl;
        return 1;
    }

    // Create a CachedDB instance
    CachedDB cached_db(base_db);

    // Write some data
    WriteOptions write_options;
    s = cached_db.Put(write_options, cached_db.DefaultColumnFamily(), "key1", "value1");
    assert(s.ok());
    s = cached_db.Put(write_options, cached_db.DefaultColumnFamily(), "key2", "value2");
    assert(s.ok());

    // Read data
    ReadOptions read_options;
    PinnableSlice value;
    std::string timestamp;
    s = cached_db.Get(read_options, cached_db.DefaultColumnFamily(), "key1", &value, &timestamp);
    if (s.ok()) {
        std::cout << "Read key1: " << value.ToString() << std::endl;
        if (!timestamp.empty()) {
            std::cout << "Timestamp: " << timestamp << std::endl;
        }
    } else {
        std::cerr << "Failed to read key1: " << s.ToString() << std::endl;
    }

    // Read the same key again (should be cached)
    value.Reset();
    s = cached_db.Get(read_options, cached_db.DefaultColumnFamily(), "key1", &value, &timestamp);
    if (s.ok()) {
        std::cout << "Read key1 again (from cache): " << value.ToString() << std::endl;
    }

    // Read a non-existent key
    value.Reset();
    s = cached_db.Get(read_options, cached_db.DefaultColumnFamily(), "key3", &value, &timestamp);
    if (s.IsNotFound()) {
        std::cout << "key3 not found, as expected" << std::endl;
    }

    // Delete a key
    s = cached_db.Delete(write_options, cached_db.DefaultColumnFamily(), "key2");
    assert(s.ok());

    // Try to read the deleted key
    value.Reset();
    s = cached_db.Get(read_options, cached_db.DefaultColumnFamily(), "key2", &value, &timestamp);
    if (s.IsNotFound()) {
        std::cout << "key2 not found after deletion, as expected" << std::endl;
    }

    base_db->Close();
    return 0;
}