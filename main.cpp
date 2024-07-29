#include <iostream>
#include <cassert>
#include <vector>
#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>
#include <rocksdb/convenience.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/utilities/backup_engine.h>

using namespace rocksdb;

void HandleError(const Status &status) {
    if (!status.ok()) {
        std::cerr << "Error: " << status.ToString() << std::endl;
        exit(1);
    }
}

int main() {
    // Open the database
    DB *db;
    Options options;
    options.create_if_missing = true;
    options.compression = kSnappyCompression;

    Status status = DB::Open(options, "/tmp/rocksdb_example", &db);
    HandleError(status);

    // Column Families
    std::vector<ColumnFamilyDescriptor> column_families;
    column_families.emplace_back(
            kDefaultColumnFamilyName, ColumnFamilyOptions());
    column_families.emplace_back(
            "new_cf", ColumnFamilyOptions());

    std::vector<ColumnFamilyHandle *> handles;
    status = db->CreateColumnFamilies(ColumnFamilyOptions(), {"new_cf"}, &handles);
    HandleError(status);

    // Basic Put operation
    status = db->Put(WriteOptions(), "key1", "value1");
    HandleError(status);

    // Get operation
    std::string value;
    status = db->Get(ReadOptions(), "key1", &value);
    HandleError(status);
    std::cout << "key1: " << value << std::endl;

    // Put in a specific column family
    status = db->Put(WriteOptions(), handles[1], "key2", "value2");
    HandleError(status);

    // Get from a specific column family
    status = db->Get(ReadOptions(), handles[1], "key2", &value);
    HandleError(status);
    std::cout << "key2 in new_cf: " << value << std::endl;

    // Iterate over the database
    Iterator *it = db->NewIterator(ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::cout << "Iterate: " << it->key().ToString() << ": " << it->value().ToString() << std::endl;
    }
    delete it;

    // Batch write
    WriteBatch batch;
    batch.Put("batch_key1", "batch_value1");
    batch.Put("batch_key2", "batch_value2");
    batch.Delete("key1");
    status = db->Write(WriteOptions(), &batch);
    HandleError(status);

    // Use snapshot for consistent read
    const Snapshot *snapshot = db->GetSnapshot();
    ReadOptions readOptions;
    readOptions.snapshot = snapshot;

    status = db->Get(readOptions, "batch_key1", &value);
    HandleError(status);
    std::cout << "Snapshot read - batch_key1: " << value << std::endl;

    db->ReleaseSnapshot(snapshot);

    // Merge operation (if supported by the merge operator)
    status = db->Merge(WriteOptions(), "merge_key", "partial_value");
    HandleError(status);

    // Backup the database
    BackupEngine *backup_engine;
    status = BackupEngine::Open(Env::Default(), BackupEngineOptions("/tmp/rocksdb_backup"), &backup_engine);
    HandleError(status);

    status = backup_engine->CreateNewBackup(db);
    HandleError(status);

    delete backup_engine;

    // Clean up
    for (auto handle: handles) {
        status = db->DestroyColumnFamilyHandle(handle);
        HandleError(status);
    }
    delete db;

    return 0;
}