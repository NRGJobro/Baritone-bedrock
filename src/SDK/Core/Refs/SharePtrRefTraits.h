#pragma once

#include "OwnerStorageSharePtr.h"
#include "StackResultStorageSharePtr.h"
#include "WeakStorageSharePtr.h"

template <typename T>
struct SharePtrRefTraits {
    using StackRef = T;
    using WeakStorage = WeakStorageSharePtr<StackRef>;
    using OwnerStorage = OwnerStorageSharePtr<StackRef>;
    using OwnerStackRef = StackRef;
    using StackResultStorage = StackResultStorageSharePtr<OwnerStackRef>;
};
