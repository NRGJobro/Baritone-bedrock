#include "MaterialPtr.h"

#include "../../../Memory/Sig/SignatureManager.h"
#include "../../../Utils/Utils.h"
#include "../../Render/RenderMaterialGroup.h"

mce::MaterialPtr* mce::MaterialPtr::createMaterial(const HashedString& name) {
    static RenderMaterialGroup* materialCreator = nullptr;

    if (materialCreator == nullptr)
        materialCreator = Utils::getFromOffset<RenderMaterialGroup*>(GET_SIG("mce::RenderMaterialGroup::common"), 3);

    return Utils::CallVFunc<1, MaterialPtr*>(materialCreator, name);
}
