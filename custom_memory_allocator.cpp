//
// Created by darion.yaphet on 2024/7/28.
//
#include <rocksdb/db.h>
#include <rocksdb/memory_allocator.h>
#include <rocksdb/table.h>

class CustomMemoryAllocator : public rocksdb::MemoryAllocator {
public:
    void* Allocate(size_t size) override {
        // 自定义内存分配逻辑
        return malloc(size);
    }

    void Deallocate(void* p) override {
        // 自定义内存释放逻辑
        free(p);
    }

    const char* Name() const override {
        return "CustomMemoryAllocator";
    }
};

int main() {
    rocksdb::Options options;
    // 创建块缓存选项
    rocksdb::LRUCacheOptions cache_opts;

    // 设置自定义内存分配器
    std::shared_ptr<CustomMemoryAllocator> custom_allocator(new CustomMemoryAllocator());
    cache_opts.memory_allocator = custom_allocator;

    // 创建块缓存
    std::shared_ptr<rocksdb::Cache> cache = rocksdb::NewLRUCache(cache_opts);

    // 创建块基础表选项
    rocksdb::BlockBasedTableOptions table_options;
    table_options.block_cache = cache;

    // 将块基础表选项设置到主选项中
    options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));

    rocksdb::DB* db;
    rocksdb::Status status = rocksdb::DB::Open(options, "/path/to/db", &db);

    return 0;
}