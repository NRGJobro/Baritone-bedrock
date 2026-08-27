#pragma once

#include "../../../Core/Refs/SharePtrRefTraits.h"
#include "../../../Core/Refs/WeakRefT.h"
#include "EntityContext.h"

struct EntityRegistryRefTraits : SharePtrRefTraits<EntityRegistry> {};
class WeakStorageEntity {
public:
    enum class EmptyInit : int {
        NoValue
    };

    enum class VariadicInit : int {
        NonAmbiguous
    };

    WeakRefT<EntityRegistryRefTraits> registry;
    EntityId entity;
};

class OwnerStorageEntity {
public:
    enum class EmptyInit : int {
        NoValue
    };

    enum class VariadicInit : int {
        NonAmbiguous
    };

    std::optional<EntityContext> context;
};

class StackResultStorageEntity {
public:
    std::optional<EntityContext> context;
};

struct EntityRefTraits {
    using StackRef = EntityContext;
    using OwnerStackRef = StackRef;
    using WeakStorage = WeakStorageEntity;
    using OwnerStorage = OwnerStorageEntity;
    using StackResultStorage = StackResultStorageEntity;
};

class WeakEntityRef : public WeakRefT<EntityRefTraits> {};
