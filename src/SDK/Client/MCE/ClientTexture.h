#pragma once

namespace mce {
    // Only the ABI footprint is needed: Limiter passes an empty texture vector
    // to Mesh::_renderMesh and never dereferences a ClientTexture resource.
    // ResourcePointer<T> is one vtable pointer plus one shared_ptr control pair
    // on x64, so preserve that 0x18-byte footprint without the unused texture
    // SDK graph.
    struct ClientTexture {
        void* vtable = nullptr;
        std::shared_ptr<void> resourcePointerBlock{};
    };

    static_assert(sizeof(ClientTexture) == 0x18);
}
