#pragma once

#include "../../DimensionID.h"
#include "MapDecoration.h"

class MapItemTrackedActor {
public:
    enum class Type {
        Entity,
        BlockEntity,
        Other
    };

    struct UniqueId {
        Type type;
        uint64_t keyEntityId;
        glm::ivec3 keyBlockPos;
    };

    UniqueId uniqueId;
    bool needsResend;
    uint32_t minDirtyX;
    uint32_t minDirtyY;
    uint32_t maxDirtyX;
    uint32_t maxDirtyY;
    int tick;
    float lastRotation;
    MapDecoration::Type decorationType;
    DimensionID dimensionId;
    std::shared_ptr<void*> chunkViewSource;
};
