#pragma once

namespace Core {
    struct FlatFileManifestInfo {
        std::string path;
        uint64_t seekPos;
        uint64_t fileSize;
        uint8_t flags;
    };
}
