#pragma once
#include "FileAccessType.h"
#include "FileImpl.h"
#include "FileStats.h"
#include "FileStorageArea.h"
#include "FlatFileSystemImpl.h"
#include "TransactionFlags.h"

namespace Core {
    class FileSystemImpl {
        void** vtable;

    public:
        std::shared_ptr<FileStorageArea> storageArea;
        bool loggingEnabled;
        bool transactionEnded;
        bool unk;
        TransactionFlags transactionFlags;
        FileAccessType accessType;
        FileStats stats;
        std::mutex fileLock;
        std::vector<FileImpl*> files;
        FlatFileSystemImpl flatFileSystem;
    };
}
