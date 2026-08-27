#pragma once

namespace Bedrock {
    struct StaticOptimizedString {
        enum class StorageType : int {
            Static,
            Dynamic
        };

        union {
            uint64_t data{};
            char bytes[8];
        };

        void _set(const char* data, uint64_t length, StorageType storageType);
        [[nodiscard]] uint64_t length() const;
        StaticOptimizedString& operator=(const StaticOptimizedString& other);
    };
}
