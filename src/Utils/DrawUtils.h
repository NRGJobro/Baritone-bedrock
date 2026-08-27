#pragma once

#include "../SDK/Client/Font/Fonts.h"
#include "../SDK/Client/MCE/MaterialPtr.h"
#include "../SDK/Render/MinecraftUIRenderContext.h"
#include "../SDK/Render/ScreenContext.h"

class DrawUtils {
public:
    static void updateMCUIRC(MinecraftUIRenderContext* ctx);
    static void update(ScreenContext* ctx);

    static mce::MaterialPtr* getUIFillColor();
    static mce::MaterialPtr* getUITextured();
    static mce::MaterialPtr* getNameTagDepthTested();
    static mce::MaterialPtr* getSignText();
    static mce::MaterialPtr* getUITextureAndColor();

    static ScreenContext* getScreenContext();
    static Tessellator* getTessellator();

    static void setShaderColor(float r = 1.f, float g = 1.f, float b = 1.f, float a = 1.f);

    static float getTextWidth(const std::string& text, float size = 1.f, Fonts font = Fonts::SmoothFontLatin);
    static float getCharWidth(char c, float size = 1.f, Fonts font = Fonts::SmoothFontLatin);
    static float getFontHeight(float size = 1.f, Fonts font = Fonts::SmoothFontLatin);
    static void drawText(const std::string& text, const glm::vec2& pos, const mce::Color& color = {}, float size = 1.f, Fonts font = Fonts::SmoothFontLatin, bool shadow = true);
    static void drawTextChroma(const std::string& text, const glm::vec2& pos, float alpha = 1.f, float size = 1.f, Fonts font = Fonts::SmoothFontLatin, bool shadow = true);

    static void addFilledRectangle(const glm::vec4& pos, const mce::Color& col, float alpha);
    static void addFilledRectangle(float x, float y, float width, float height, const mce::Color& col, float alpha);

    static void addFilledRectangleChroma(const glm::vec4& pos, float alpha);
    static void addFilledRectangleChroma(float x, float y, float width, float height, float alpha);

    static void addCircle(const glm::vec2& pos, float stepSize, const mce::Color& color = {}, float radius = 4.2f);

    static void addPartialCircle(const glm::vec2& pos, float fromDeg, float toDeg, float stepSize, const mce::Color& color = {}, float radius = 4.2f);

    static void addOutlinedPartialCircle(const glm::vec2& pos, float fromDeg, float toDeg, float stepSize, const mce::Color& color = {}, const mce::Color& outline = {},
        float outlineSize = 2.f, float radius = 4.2f);

    static void addOutlinedPartialCircleBlend(const glm::vec2& pos, float fromDeg, float toDeg, float stepSize, const mce::Color& color = {}, const mce::Color& outline = {},
        float outlineSize = 2.f, float radius = 4.2f, float blend = 0.75f);

    static void addOutlinedPartialCircleBlendChroma(const glm::vec2& pos, float fromDeg, float toDeg, float stepSize, const float alpha = 0.45f, const float outlineAlpha = 1.f,
        float outlineSize = 2.f, float radius = 4.2f, float blend = 0.75f);

    static void addRoundedRectangle(float x, float y, float width, float height, float stepSize, const mce::Color& color = {}, uint8_t flags = 0xF, float radius = 4.2f);

    static void addRoundedOutlinedRectangle(float x, float y, float width, float height, float stepSize, const mce::Color& color = {}, const mce::Color& outline = {},
        float outlineSize = 2.f, uint8_t flags = 0xF, float radius = 4.2f);

    static void addRoundedOutlinedRectangleBlend(float x, float y, float width, float height, float stepSize, const mce::Color& color = {}, const mce::Color& outline = {},
        float outlineSize = 2.f, uint8_t flags = 0xF, float radius = 4.2f, float blend = 0.75f);

    static void addRoundedOutlinedRectangleBlendChroma(float x, float y, float width, float height, float stepSize, float alpha = 0.45f, float outlineAlpha = 1.f,
        float outlineSize = 2.f, uint8_t flags = 0xF, float radius = 4.2f, float blend = 0.75f);
};
