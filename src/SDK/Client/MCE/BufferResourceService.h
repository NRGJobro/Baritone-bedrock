#pragma once

namespace mce {
    // Limiter only stores/copies Minecraft's shared/weak references to this
    // service; it never reads service fields. Keeping it opaque avoids pulling
    // the entire unused RenderContext/resource-tracker SDK graph into the
    // client while preserving pointer/control-block ABI.
    struct BufferResourceService {};
}
