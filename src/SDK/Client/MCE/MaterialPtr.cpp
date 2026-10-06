#include "MaterialPtr.h"

#include "../../../Memory/Sig/SignatureManager.h"
#include "../../../Utils/Utils.h"

struct RenderMaterialGroup;

std::shared_ptr<mce::MaterialPtr> mce::MaterialPtr::createMaterial(const HashedString& name) {
    static RenderMaterialGroup* materialCreator = nullptr;

    if (materialCreator == nullptr)
        materialCreator = Utils::getFromOffset<RenderMaterialGroup*>(GET_SIG("mce::RenderMaterialGroup::common"), 3);

    if (materialCreator == nullptr)
        return nullptr;

    return Utils::CallVFunc<1, std::shared_ptr<MaterialPtr>, const HashedString&>(
        materialCreator, name);
}
