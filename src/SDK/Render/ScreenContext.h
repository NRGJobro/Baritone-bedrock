#pragma once

#include "../Bedrock/NonOwnerPointer.h"
#include "../Client/MCE/MeshContext.h"
#include "UIScreenContext.h"

class GuiData;
class UIProfanityContext;
class Tessellator;
struct ShaderColor;
namespace mce { struct Clock; struct ViewportInfo; }

class ScreenContext : public UIScreenContext, public mce::MeshContext {
public:
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
