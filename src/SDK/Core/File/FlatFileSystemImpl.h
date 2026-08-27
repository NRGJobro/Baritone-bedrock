#pragma once

#include "FlatFileManifestTracker.h"

namespace Core {
    struct FlatFileSystemImpl {
        FileSystemImpl* fileSystemImpl;
        std::shared_ptr<FlatFileManifestTracker> flatFileManifestTracker;
    };
}
