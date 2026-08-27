#pragma once

namespace Core {
    struct FileStats {
        std::atomic<uint64_t> numSuccessfulWriteOperations;
        std::atomic<uint64_t> numBytesWritten;
        std::atomic<uint64_t> numFailedWriteOperations;
        std::atomic<uint64_t> numSuccessfulReadOperations;
        std::atomic<uint64_t> numBytesRead;
        std::atomic<uint64_t> numFailedReadOperations;
        std::atomic<uint64_t> fileSystemSize;
        std::atomic<uint64_t> fileSystemAllocatedSize;
    };
}
