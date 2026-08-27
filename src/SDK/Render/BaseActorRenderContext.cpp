#include "BaseActorRenderContext.h"

#include "../../Memory/Sig/SignatureManager.h"

BaseActorRenderContext::BaseActorRenderContext(ScreenContext* screenContext, ClientInstance* clientInstance, MinecraftGame* minecraftGame) {
    using func_t = void(*)(BaseActorRenderContext*, ScreenContext*, ClientInstance*, MinecraftGame*);
    static auto func = reinterpret_cast<func_t>(GET_SIG("BaseActorRenderContext::BaseActorRenderContext"));
    func(this, screenContext, clientInstance, minecraftGame);
}

ItemRenderer* BaseActorRenderContext::getItemRenderer() {
    return hat::member_at<ItemRenderer*>(this, 0x58);
}
