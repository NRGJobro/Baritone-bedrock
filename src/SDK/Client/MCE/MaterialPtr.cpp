#include "MaterialPtr.h"

#include "../../../Memory/Sig/SignatureManager.h"
#include "../../../Utils/Utils.h"

struct RenderMaterialGroup {};

std::shared_ptr<mce::MaterialPtr> mce::MaterialPtr::createMaterial(const HashedString& name) {
    static RenderMaterialGroup* materialCreator = nullptr;

    if (materialCreator == nullptr)
        materialCreator = Utils::getFromOffset<RenderMaterialGroup*>(GET_SIG("mce::RenderMaterialGroup::common"), 3);

    if (materialCreator == nullptr)
        return nullptr;

    // This is a virtual member returning a non-trivial type. Calling it through
    // the generic free-function-shaped CallVFunc helper is ABI-invalid on
    // Windows x64: a free function puts the hidden shared_ptr return buffer in
    // RCX, while a member function keeps `this` in RCX and uses RDX for that
    // buffer. The old call therefore handed Minecraft the return buffer as
    // RenderMaterialGroup::this and crashed inside its registry lookup.
    using CreateMaterial = std::shared_ptr<MaterialPtr>
        (RenderMaterialGroup::*)(const HashedString&);
    static_assert(sizeof(CreateMaterial) == sizeof(void*));

    const auto target = (*reinterpret_cast<void***>(materialCreator))[1];
    CreateMaterial createMaterial{};
    std::memcpy(&createMaterial, &target, sizeof(target));
    return (materialCreator->*createMaterial)(name);
}
