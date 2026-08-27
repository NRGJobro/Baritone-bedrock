#pragma once

#include "FlatFileManifest.h"

namespace Core {
    struct FlatFileManifestTracker {
        std::mutex manifestsLock;
        std::unordered_map<std::string, std::shared_ptr<FlatFileManifest>> manifestMap;
        std::set<std::string> manifestNames;
    };
}
