#pragma once

class ShaderColor;
namespace mce {
    class Camera;
    class RenderContext;
}

namespace mce {
    class MeshContext {
    public:
        RenderContext* renderContext;
        Camera* camera;
        void* constantBuffers;
        void* constantBufferManager;
        ShaderColor* currentShaderColor;
        ShaderColor* currentShaderDarkColor;
        void* bufferResourceService;
        void* currentQuadIndexBuffer;
        void** immediateBufferVtable;
        std::shared_ptr<int64_t> immediateBuffer;
        std::optional<glm::vec4> normalizedClipRegion;
        uint8_t subClientId;
        bool isDrawingUI;
        bool isDrawingFirstPersonObjects;
        bool isDrawingEnvironmentalText;
        bool isDrawingPersistentUIElement;
        bool isDoingFrameCapture;
        bool isDrawingEditorSelectionHighlight;
        bool isDrawingMovingBlock;
        bool isDitheringEnabled;
        bool isAlphaMaskedTintEnabled;

        void setClippingRectangle(float x, float y, float width, float height);
        void resetClippingRectangle();
    };
}
