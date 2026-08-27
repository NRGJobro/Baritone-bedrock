#include "ScreenContext.h"

mce::MeshContext* ScreenContext::toMeshContext() {
    return reinterpret_cast<MeshContext*>(reinterpret_cast<uintptr_t>(this) + 0x10);
}
