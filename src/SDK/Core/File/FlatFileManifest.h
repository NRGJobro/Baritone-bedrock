#pragma once

#include "FlatFileManifestInfo.h"

namespace Core {
    struct FlatFileManifest {
        std::unordered_map<std::string, uint64_t> manifestEntriesMap;
        std::vector<FlatFileManifestInfo> manifestInfoVector;
        uint64_t entriesCount;
        uint64_t version;
        std::string manifestPath;
    };
}
