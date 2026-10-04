#pragma once

#include "../Bedrock/NonOwnerPointer.h"
#include "../Client/GUI/GuiData.h"
#include "../Client/MCE/Clock.h"
#include "../Client/MCE/MeshContext.h"
#include "../Screen/UIProfanityContext.h"
#include "ShaderColor.h"
#include "Tessellator.h"
#include "UIScreenContext.h"

class ScreenContext : public UIScreenContext, public mce::MeshContext {
public:
    // idk what was removed
    //void* renderDevice;
    //void* renderSettings;
    void* frameBufferObject;
    mce::ViewportInfo* viewport;
    Bedrock::NonOwnerPointerRef<GuiData> guiData;
    mce::Clock* clock;
    Tessellator* tessellator;
    void* minecraftGraphicsPipeline;
    Bedrock::NonOwnerPointerRef<int64_t> minecraftGraphics;
    Bedrock::NonOwnerPointerRef<UIProfanityContext> uiProfanityContext;
    void* unkPtr;
    void* commandListQueue;
    void* frameAllocator;

    MeshContext* toMeshContext();
    Tessellator* getTessellator() { return hat::member_at<Tessellator*>(this, 0xB8); }
    ShaderColor* getShaderColor() { return hat::member_at<ShaderColor*>(toMeshContext(), 0x20); }
};
