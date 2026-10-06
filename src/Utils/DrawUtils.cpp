#include "DrawUtils.h"

#include "../SDK/Client/Font/Font.h"
#include "../SDK/Client/MCE/MaterialPtr.h"
#include "../SDK/Client/MinecraftGame.h"
#include "../SDK/MC.h"
#include "../SDK/Render/CaretMeasureData.h"
#include "../SDK/Render/MinecraftUIRenderContext.h"
#include "../SDK/Render/ScreenContext.h"
#include "../SDK/Render/Tessellator.h"
#include "../SDK/Render/TextMeasureData.h"
#include "TimeUtils.h"
#include "Utils.h"

constexpr float RAD_DEG = 3.1415927f / 180.f;

static mce::MaterialPtr* uiFillColor;
static mce::MaterialPtr* selectionOverlay;

static MinecraftUIRenderContext* renderCtx;
static ScreenContext* screenContext;
static Tessellator* tessellator;

void DrawUtils::updateMCUIRC(MinecraftUIRenderContext* ctx) {
    if (ctx == nullptr)
        return;
    renderCtx = ctx;

    update(ctx->screenContext);
}

void DrawUtils::update(ScreenContext* ctx) {
    if (ctx == nullptr)
        return;
    screenContext = ctx;
    tessellator = ctx->getTessellator();

    if (uiFillColor == nullptr)
        uiFillColor = mce::MaterialPtr::createMaterial("ui_fill_color");

    if (selectionOverlay == nullptr)
        selectionOverlay = mce::MaterialPtr::createMaterial("selection_overlay");
}

void DrawUtils::reset() {
    renderCtx = nullptr;
    screenContext = nullptr;
    tessellator = nullptr;
    uiFillColor = nullptr;
    selectionOverlay = nullptr;
}

mce::MaterialPtr* DrawUtils::getUIFillColor() {
    return uiFillColor;
}

ScreenContext* DrawUtils::getScreenContext() {
    return screenContext;
}

mce::MaterialPtr* DrawUtils::getSelectionOverlay() {
    return selectionOverlay;
}

Tessellator* DrawUtils::getTessellator() {
    return tessellator;
}

void DrawUtils::setShaderColor(const float r, const float g, const float b, const float a) {
    if (screenContext == nullptr)
        return;

    const auto shaderColor = screenContext->getShaderColor();
    if (shaderColor == nullptr)
        return;

    shaderColor->color.r = r;
    shaderColor->color.g = g;
    shaderColor->color.b = b;
    shaderColor->color.a = a;
    shaderColor->dirty = true;
}

float DrawUtils::getTextWidth(const std::string& text, const float size, const Fonts font) {
    const auto game = MC::getMinecraftGame();
    const auto f = game == nullptr ? nullptr : game->getFont(font);

    return f == nullptr ? 0.f : f->getLineLength(text, size);
}

float DrawUtils::getCharWidth(const char c, const float size, const Fonts font) {
    return getTextWidth(std::string(1, c), size, font);
}

float DrawUtils::getFontHeight(const float size, const Fonts font) {
    const auto game = MC::getMinecraftGame();
    const auto f = game == nullptr ? nullptr : game->getFont(font);

    return f == nullptr ? 0.f : f->getLineHeight() * size;
}

void DrawUtils::drawText(const std::string& text, const glm::vec2& pos, const mce::Color& color, const float size, const Fonts font, const bool shadow) {
    if (renderCtx == nullptr)
        return;

    const auto game = MC::getMinecraftGame();
    const auto f = game == nullptr ? nullptr : game->getFont(font);
    if (f == nullptr)
        return;

    const glm::vec4 bounds{pos.x, pos.x + 1000.f, pos.y, pos.y + 1000.f};
    const TextMeasureData measure{size, 0.f, shadow, false, false};
    const CaretMeasureData caret{20, false};
    renderCtx->drawText(f, bounds, text, color, color.a, 0.f, measure, caret);
}

void DrawUtils::drawTextChroma(const std::string& text, const glm::vec2& pos, const float alpha, const float size, const Fonts font, const bool shadow) {
    if (renderCtx == nullptr)
        return;

    float x = 0.f;

    for (const auto c : text) {
        const auto l = TimeUtils::currentTimeMillis() - (static_cast<millis>(pos.x + x) * 10 - static_cast<millis>(pos.y) * 10);
        const auto color = Utils::HSVtoRGB(static_cast<float>(l % 2000) / 2000.f, 0.8f, 0.8f);

        drawText(std::string(1, c), {pos.x + x, pos.y}, {color, alpha}, size, font, shadow);

        x += getCharWidth(c, size, font);
    }
}

void DrawUtils::addFilledRectangle(const glm::vec4& pos, const mce::Color& col, const float alpha) {
    if (tessellator == nullptr)
        return;

    tessellator->color(col.r, col.g, col.b, alpha);

    tessellator->vertex(pos.x, pos.w);
    tessellator->vertex(pos.z, pos.w);
    tessellator->vertex(pos.z, pos.y);

    tessellator->vertex(pos.z, pos.y);
    tessellator->vertex(pos.x, pos.y);
    tessellator->vertex(pos.x, pos.w);
}

void DrawUtils::addFilledRectangle(const float x, const float y, const float width, const float height, const mce::Color& col, const float alpha) {
    if (tessellator == nullptr)
        return;

    tessellator->color(col.r, col.g, col.b, alpha);

    tessellator->vertex(x, y);
    tessellator->vertex(x, y + height);
    tessellator->vertex(x + width, y);

    tessellator->vertex(x, y + height);
    tessellator->vertex(x + width, y + height);
    tessellator->vertex(x + width, y);
}

void DrawUtils::addFilledRectangleChroma(const glm::vec4& pos, const float alpha) {
    if (tessellator == nullptr)
        return;

    tessellator->vertexChroma(pos.x, pos.w, alpha);
    tessellator->vertexChroma(pos.z, pos.w, alpha);
    tessellator->vertexChroma(pos.z, pos.y, alpha);

    tessellator->vertexChroma(pos.z, pos.y, alpha);
    tessellator->vertexChroma(pos.x, pos.y, alpha);
    tessellator->vertexChroma(pos.x, pos.w, alpha);
}

void DrawUtils::addFilledRectangleChroma(const float x, const float y, const float width, const float height, const float alpha) {
    if (tessellator == nullptr)
        return;

    tessellator->vertexChroma(x, y, alpha);
    tessellator->vertexChroma(x, y + height, alpha);
    tessellator->vertexChroma(x + width, y, alpha);

    tessellator->vertexChroma(x, y + height, alpha);
    tessellator->vertexChroma(x + width, y + height, alpha);
    tessellator->vertexChroma(x + width, y, alpha);
}

void DrawUtils::addCircle(const glm::vec2& pos, const float stepSize, const mce::Color& color, const float radius) {
    if (tessellator == nullptr)
        return;

    for (float f = 0.f; f < 360.f; f += stepSize) {
        const float rad = f * RAD_DEG;
        const float rad2 = std::min(f + stepSize, 360.f) * RAD_DEG;

        const float sin = sinf(rad);
        const float sin2 = sinf(rad2);
        const float cos = cosf(rad);
        const float cos2 = cosf(rad2);

        tessellator->color(color);

        tessellator->vertex(cos2 * radius + pos.x, sin2 * radius + pos.y);
        tessellator->vertex(cos * radius + pos.x, sin * radius + pos.y);
        tessellator->vertex(pos.x, pos.y);
    }
}

void DrawUtils::addPartialCircle(const glm::vec2& pos, float fromDeg, float toDeg, const float stepSize, const mce::Color& color, const float radius) {
    if (tessellator == nullptr || fromDeg == toDeg)
        return;

    if (fromDeg > toDeg) {
        const float tmp = fromDeg;
        fromDeg = toDeg;
        toDeg = tmp;
    }

    tessellator->color(color);

    for (float f = fromDeg; f < toDeg; f += stepSize) {
        const float rad = f * RAD_DEG;
        const float rad2 = std::min(f + stepSize, toDeg) * RAD_DEG;

        const float sin = sinf(rad);
        const float sin2 = sinf(rad2);
        const float cos = cosf(rad);
        const float cos2 = cosf(rad2);

        tessellator->vertex(cos2 * radius + pos.x, sin2 * radius + pos.y, 0.f);
        tessellator->vertex(cos * radius + pos.x, sin * radius + pos.y, 0.f);
        tessellator->vertex(pos.x, pos.y, 0.f);
    }
}

void DrawUtils::addOutlinedPartialCircle(const glm::vec2& pos, float fromDeg, float toDeg, const float stepSize, const mce::Color& color, const mce::Color& outline,
    const float outlineSize, const float radius) {
    if (tessellator == nullptr || fromDeg == toDeg)
        return;

    if (fromDeg > toDeg) {
        const float tmp = fromDeg;
        fromDeg = toDeg;
        toDeg = tmp;
    }

    for (float f = fromDeg; f < toDeg; f += stepSize) {
        const float rad = f * RAD_DEG;
        const float rad2 = std::min(f + stepSize, toDeg) * RAD_DEG;

        const float sin = sinf(rad);
        const float sin2 = sinf(rad2);
        const float cos = cosf(rad);
        const float cos2 = cosf(rad2);

        tessellator->color(outline);

        tessellator->vertex(cos * radius + pos.x, sin * radius + pos.y);
        tessellator->vertex(cos * (radius - outlineSize) + pos.x, sin * (radius - outlineSize) + pos.y);
        tessellator->vertex(cos2 * radius + pos.x, sin2 * radius + pos.y);

        tessellator->vertex(cos * (radius - outlineSize) + pos.x, sin * (radius - outlineSize) + pos.y);
        tessellator->vertex(cos2 * (radius - outlineSize) + pos.x, sin2 * (radius - outlineSize) + pos.y);
        tessellator->vertex(cos2 * radius + pos.x, sin2 * radius + pos.y);

        tessellator->color(color);

        tessellator->vertex(cos2 * (radius - outlineSize) + pos.x, sin2 * (radius - outlineSize) + pos.y);
        tessellator->vertex(cos * (radius - outlineSize) + pos.x, sin * (radius - outlineSize) + pos.y);
        tessellator->vertex(pos.x, pos.y);
    }
}

void DrawUtils::addOutlinedPartialCircleBlend(const glm::vec2& pos, float fromDeg, float toDeg, const float stepSize, const mce::Color& color, const mce::Color& outline,
    const float outlineSize, const float radius, const float blend) {
    if (tessellator == nullptr || fromDeg == toDeg)
        return;

    if (fromDeg > toDeg) {
        const float tmp = fromDeg;
        fromDeg = toDeg;
        toDeg = tmp;
    }

    const float halfBlend = blend / 2.f;

    for (float f = fromDeg; f < toDeg; f += stepSize) {
		const float rad = f * RAD_DEG;
		const float rad2 = std::min(f + stepSize, toDeg) * RAD_DEG;

		const float sin = sinf(rad);
		const float sin2 = sinf(rad2);
		const float cos = cosf(rad);
		const float cos2 = cosf(rad2);

		tessellator->color(outline);

		tessellator->vertex(cos * radius + pos.x, sin * radius + pos.y, 0.f);
		tessellator->vertex(cos * (radius - outlineSize + halfBlend) + pos.x, sin * (radius - outlineSize + halfBlend) + pos.y, 0.f);
		tessellator->vertex(cos2 * radius + pos.x, sin2 * radius + pos.y, 0.f);

		tessellator->vertex(cos * (radius - outlineSize + halfBlend) + pos.x, sin * (radius - outlineSize + halfBlend) + pos.y, 0.f);
		tessellator->vertex(cos2 * (radius - outlineSize + halfBlend) + pos.x, sin2 * (radius - outlineSize + halfBlend) + pos.y, 0.f);
		tessellator->vertex(cos2 * radius + pos.x, sin2 * radius + pos.y, 0.f);

		tessellator->color(color);

		tessellator->vertex(cos2 * (radius - outlineSize - halfBlend) + pos.x, sin2 * (radius - outlineSize - halfBlend) + pos.y, 0.f);
		tessellator->vertex(cos * (radius - outlineSize - halfBlend) + pos.x, sin * (radius - outlineSize - halfBlend) + pos.y, 0.f);
		tessellator->vertex(pos.x, pos.y, 0.f);


		tessellator->color(outline);
		tessellator->vertex(cos * (radius - outlineSize + halfBlend) + pos.x, sin * (radius - outlineSize + halfBlend) + pos.y, 0.f);
		tessellator->color(color);
		tessellator->vertex(cos * (radius - outlineSize - halfBlend) + pos.x, sin * (radius - outlineSize - halfBlend) + pos.y, 0.f);
		tessellator->color(outline);
		tessellator->vertex(cos2 * (radius - outlineSize + halfBlend) + pos.x, sin2 * (radius - outlineSize + halfBlend) + pos.y, 0.f);

		tessellator->color(color);
		tessellator->vertex(cos * (radius - outlineSize - halfBlend) + pos.x, sin * (radius - outlineSize - halfBlend) + pos.y, 0.f);
		tessellator->vertex(cos2 * (radius - outlineSize - halfBlend) + pos.x, sin2 * (radius - outlineSize - halfBlend) + pos.y, 0.f);
		tessellator->color(outline);
		tessellator->vertex(cos2 * (radius - outlineSize + halfBlend) + pos.x, sin2 * (radius - outlineSize + halfBlend) + pos.y, 0.f);


		tessellator->color(outline, 0.f);
		tessellator->vertex(cos * (radius + blend) + pos.x, sin * (radius + blend) + pos.y, 0.f);
		tessellator->color(outline);
		tessellator->vertex(cos * radius + pos.x, sin * radius + pos.y, 0.f);
		tessellator->color(outline, 0.f);
		tessellator->vertex(cos2 * (radius + blend) + pos.x, sin2 * (radius + blend) + pos.y, 0.f);

		tessellator->color(outline);
		tessellator->vertex(cos * radius + pos.x, sin * radius + pos.y, 0.f);
		tessellator->vertex(cos2 * radius + pos.x, sin2 * radius + pos.y, 0.f);
		tessellator->color(outline, 0.f);
		tessellator->vertex(cos2 * (radius + blend) + pos.x, sin2 * (radius + blend) + pos.y, 0.f);
    }
}

void DrawUtils::addOutlinedPartialCircleBlendChroma(const glm::vec2& pos, float fromDeg, float toDeg, const float stepSize, const float alpha, const float outlineAlpha,
    const float outlineSize, const float radius, const float blend) {
    if (tessellator == nullptr || fromDeg == toDeg)
        return;

    if (fromDeg > toDeg) {
        const float tmp = fromDeg;
        fromDeg = toDeg;
        toDeg = tmp;
    }

    const float halfBlend = blend / 2.f;

    for (float f = fromDeg; f < toDeg; f += stepSize) {
		const float rad = f * RAD_DEG;
		const float rad2 = std::min(f + stepSize, toDeg) * RAD_DEG;

		const float sin = sinf(rad);
		const float sin2 = sinf(rad2);
		const float cos = cosf(rad);
		const float cos2 = cosf(rad2);

		tessellator->vertexChroma(cos * radius + pos.x, sin * radius + pos.y, outlineAlpha);
		tessellator->vertexChroma(cos * (radius - outlineSize + halfBlend) + pos.x, sin * (radius - outlineSize + halfBlend) + pos.y, outlineAlpha);
		tessellator->vertexChroma(cos2 * radius + pos.x, sin2 * radius + pos.y, outlineAlpha);

		tessellator->vertexChroma(cos * (radius - outlineSize + halfBlend) + pos.x, sin * (radius - outlineSize + halfBlend) + pos.y, outlineAlpha);
		tessellator->vertexChroma(cos2 * (radius - outlineSize + halfBlend) + pos.x, sin2 * (radius - outlineSize + halfBlend) + pos.y, outlineAlpha);
		tessellator->vertexChroma(cos2 * radius + pos.x, sin2 * radius + pos.y, outlineAlpha);


		tessellator->vertexChroma(cos2 * (radius - outlineSize - halfBlend) + pos.x, sin2 * (radius - outlineSize - halfBlend) + pos.y, alpha);
		tessellator->vertexChroma(cos * (radius - outlineSize - halfBlend) + pos.x, sin * (radius - outlineSize - halfBlend) + pos.y, alpha);
		tessellator->vertexChroma(pos.x, pos.y, alpha);


		tessellator->vertexChroma(cos * (radius - outlineSize + halfBlend) + pos.x, sin * (radius - outlineSize + halfBlend) + pos.y, outlineAlpha);
		tessellator->vertexChroma(cos * (radius - outlineSize - halfBlend) + pos.x, sin * (radius - outlineSize - halfBlend) + pos.y, alpha);
		tessellator->vertexChroma(cos2 * (radius - outlineSize + halfBlend) + pos.x, sin2 * (radius - outlineSize + halfBlend) + pos.y, outlineAlpha);

		tessellator->vertexChroma(cos * (radius - outlineSize - halfBlend) + pos.x, sin * (radius - outlineSize - halfBlend) + pos.y, alpha);
		tessellator->vertexChroma(cos2 * (radius - outlineSize - halfBlend) + pos.x, sin2 * (radius - outlineSize - halfBlend) + pos.y, alpha);
		tessellator->vertexChroma(cos2 * (radius - outlineSize + halfBlend) + pos.x, sin2 * (radius - outlineSize + halfBlend) + pos.y, outlineAlpha);


		tessellator->vertexChroma(cos * (radius + blend) + pos.x, sin * (radius + blend) + pos.y, 0.f);
		tessellator->vertexChroma(cos * radius + pos.x, sin * radius + pos.y, outlineAlpha);
		tessellator->vertexChroma(cos2 * (radius + blend) + pos.x, sin2 * (radius + blend) + pos.y, 0.f);

		tessellator->vertexChroma(cos * radius + pos.x, sin * radius + pos.y, outlineAlpha);
		tessellator->vertexChroma(cos2 * radius + pos.x, sin2 * radius + pos.y, outlineAlpha);
		tessellator->vertexChroma(cos2 * (radius + blend) + pos.x, sin2 * (radius + blend) + pos.y, 0.f);
    }
}

void DrawUtils::addRoundedRectangle(const float x, const float y, const float width, const float height, const float stepSize,
    const mce::Color& color, const uint8_t flags, const float radius) {
    if (tessellator == nullptr)
        return;

    const bool ctl = flags & 8; // Top left corner, renders rounded if true
	const bool ctr = flags & 4; // Top right corner, renders rounded if true
	const bool cbl = flags & 2; // Bottom left corner, renders rounded if true
	const bool cbr = flags & 1; // Bottom right corner, renders rounded if true

	const float offset = std::min(std::min(radius, width / 2.f), height / 2.f);
	const glm::vec4 pos1(x, y + offset, x + width, y + height - offset);
	const glm::vec4 pos2(x + offset, y, x + width - offset, y + offset);
	const glm::vec4 pos3(x + offset, y + height - offset, x + width - offset, y + height);

	addFilledRectangle(pos1, color, color.a);
    addFilledRectangle(pos2, color, color.a);
    addFilledRectangle(pos3, color, color.a);

	if (cbr)
	    addPartialCircle({x + width - offset, y + height - offset}, 0.f, 90.f, stepSize, color, offset);
    else
        addFilledRectangle(x + width - offset, y + height - offset, offset, offset, color, color.a);

	if (cbl)
	    addPartialCircle({x + offset, y + height - offset}, 90.f, 180.f, stepSize, color, offset);
    else
        addFilledRectangle(x, y + height - offset, offset, offset, color, color.a);

	if (ctl)
	    addPartialCircle({x + offset, y + offset}, 180.f, 270.f, stepSize, color, offset);
    else
        addFilledRectangle(x, y, offset, offset, color, color.a);

	if (ctr)
	    addPartialCircle({x + width - offset, y + offset}, 270.f, 360.f, stepSize, color, offset);
    else
        addFilledRectangle(x + width - offset, y, offset, offset, color, color.a);
}

void DrawUtils::addRoundedOutlinedRectangle(float x, float y, const float width, const float height, const float stepSize, const mce::Color& color,
    const mce::Color& outline, const float outlineSize, const uint8_t flags, const float radius) {
    if (tessellator == nullptr)
        return;

    if (outlineSize == 0.f) {
        addRoundedRectangle(x, y, width, height, stepSize, color, flags, radius);
        return;
    }

    const bool ctl = flags & 8; // Top left corner, renders rounded if true
    const bool ctr = flags & 4; // Top right corner, renders rounded if true
    const bool cbl = flags & 2; // Bottom left corner, renders rounded if true
    const bool cbr = flags & 1; // Bottom right corner, renders rounded if true

	const float offset = std::min(std::min(radius, width / 2.f), height / 2.f);

    // Left
	addFilledRectangle({x, y + offset, x + outlineSize, y + height - offset}, outline, outline.a);
	addFilledRectangle({x + outlineSize, y + offset, x + offset, y + height - offset}, color, color.a);

    // Right
    addFilledRectangle({x + width - outlineSize, y + offset, x + width, y + height - offset}, outline, outline.a);
    addFilledRectangle({x + width - offset, y + offset, x + width - outlineSize, y + height - offset}, color, color.a);

    // Top
	addFilledRectangle({x + offset, y, x + width - offset, y + outlineSize}, outline, outline.a);
	addFilledRectangle({x + offset, y + outlineSize, x + width - offset, y + offset}, color, color.a);

    // Bottom
	addFilledRectangle({x + offset, y + height - outlineSize, x + width - offset, y + height}, outline, outline.a);
	addFilledRectangle({x + offset, y + height - offset, x + width - offset, y + height - outlineSize}, color, color.a);

    // Middle
    addFilledRectangle({x + offset, y + offset, x + width - offset, y + height - offset}, color, color.a);

	if (cbr)
	    addOutlinedPartialCircle({x + width - offset, y + height - offset}, 0.f, 90.f, stepSize, color, outline, outlineSize, offset);
    else {
        addFilledRectangle(x + width - offset, y + height - offset, offset - outlineSize, offset - outlineSize, color, color.a);
        addFilledRectangle(x + width - outlineSize, y + height - offset, outlineSize, offset, outline, outline.a);
        addFilledRectangle(x + width - offset, y + height - outlineSize, offset - outlineSize, outlineSize, outline, outline.a);
    }

	if (cbl)
	    addOutlinedPartialCircle({x + offset, y + height - offset}, 90.f, 180.f, stepSize, color, outline, outlineSize, offset);
    else {
        addFilledRectangle(x + outlineSize, y + height - offset, offset - outlineSize, offset - outlineSize, color, color.a);
        addFilledRectangle(x, y + height - offset, outlineSize, offset, outline, outline.a);
        addFilledRectangle(x + outlineSize, y + height - outlineSize, offset - outlineSize, outlineSize, outline, outline.a);
    }

	if (ctl)
	    addOutlinedPartialCircle({x + offset, y + offset}, 180.f, 270.f, stepSize, color, outline, outlineSize, offset);
    else {
        addFilledRectangle(x + outlineSize, y + outlineSize, offset - outlineSize, offset - outlineSize, color, color.a);
        addFilledRectangle(x, y, outlineSize, offset, outline, outline.a);
        addFilledRectangle(x + outlineSize, y, offset - outlineSize, outlineSize, outline, outline.a);
    }

	if (ctr)
	    addOutlinedPartialCircle({x + width - offset, y + offset}, 270.f, 360.f, stepSize, color, outline, outlineSize, offset);
    else {
        addFilledRectangle(x + width - offset, y + outlineSize, offset - outlineSize, offset - outlineSize, color, color.a);
        addFilledRectangle(x + width - offset, y, offset, outlineSize, outline, outline.a);
        addFilledRectangle(x + width - outlineSize, y + outlineSize, outlineSize, offset - outlineSize, outline, outline.a);
    }
}

void DrawUtils::addRoundedOutlinedRectangleBlend(const float x, const float y, const float width, const float height, const float stepSize,
    const mce::Color& color, const mce::Color& outline, const float outlineSize, const uint8_t flags, const float radius, const float blend) {
    if (tessellator == nullptr)
        return;

    if (blend == 0.f) {
        addRoundedOutlinedRectangle(x, y, width, height, stepSize, color, outline, outlineSize, flags, radius);
        return;
    }

    const bool ctl = flags & 8; // Top left corner, renders rounded if true
    const bool ctr = flags & 4; // Top right corner, renders rounded if true
    const bool cbl = flags & 2; // Bottom left corner, renders rounded if true
    const bool cbr = flags & 1; // Bottom right corner, renders rounded if true

	const float offset = std::min(std::min(radius, width / 2.f), height / 2.f);
    const float halfBlend = blend / 2.f;

	{ // Left
	    addFilledRectangle({x, y + offset, x + outlineSize - halfBlend, y + height - offset}, outline, outline.a);
	    addFilledRectangle({x + outlineSize + halfBlend, y + offset, x + offset, y + height - offset}, color, color.a);

	    tessellator->color(outline);
	    tessellator->vertex(x + outlineSize - halfBlend, y + offset, 0.f);
	    tessellator->vertex(x + outlineSize - halfBlend, y + height - offset, 0.f);
	    tessellator->color(color);
	    tessellator->vertex(x + outlineSize + halfBlend, y + offset, 0.f);

	    tessellator->color(outline);
	    tessellator->vertex(x + outlineSize - halfBlend, y + height - offset, 0.f);
	    tessellator->color(color);
	    tessellator->vertex(x + outlineSize + halfBlend, y + height - offset, 0.f);
	    tessellator->vertex(x + outlineSize + halfBlend, y + offset, 0.f);


	    tessellator->color(outline, 0.f);
	    tessellator->vertex(x - blend, y + offset, 0.f);
	    tessellator->vertex(x - blend, y + height - offset, 0.f);
	    tessellator->color(outline);
	    tessellator->vertex(x, y + offset, 0.f);

	    tessellator->color(outline, 0.f);
	    tessellator->vertex(x - blend, y + height - offset, 0.f);
	    tessellator->color(outline);
	    tessellator->vertex(x, y + height - offset, 0.f);
	    tessellator->vertex(x, y + offset, 0.f);
	}

    { // Right
        addFilledRectangle({x + width - outlineSize + halfBlend, y + offset, x + width, y + height - offset}, outline, outline.a);
        addFilledRectangle({x + width - offset, y + offset, x + width - outlineSize - halfBlend, y + height - offset}, color, color.a);

        tessellator->color(color);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + offset, 0.f);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + height - offset, 0.f);
        tessellator->color(outline);
        tessellator->vertex(x + width - outlineSize + halfBlend, y + offset, 0.f);

        tessellator->color(color);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + height - offset, 0.f);
        tessellator->color(outline);
        tessellator->vertex(x + width - outlineSize + halfBlend, y + height - offset, 0.f);
        tessellator->vertex(x + width - outlineSize + halfBlend, y + offset, 0.f);

        tessellator->color(outline);
        tessellator->vertex(x + width, y + offset, 0.f);
        tessellator->vertex(x + width, y + height - offset, 0.f);
        tessellator->color(outline, 0.f);
        tessellator->vertex(x + width + blend, y + offset, 0.f);

        tessellator->color(outline);
        tessellator->vertex(x + width, y + height - offset, 0.f);
        tessellator->color(outline, 0.f);
        tessellator->vertex(x + width + blend, y + height - offset, 0.f);
        tessellator->vertex(x + width + blend, y + offset, 0.f);
    }

	{ // Top
	    addFilledRectangle({x + offset, y, x + width - offset, y + outlineSize - halfBlend}, outline, outline.a);
	    addFilledRectangle({x + offset, y + outlineSize + halfBlend, x + width - offset, y + offset}, color, color.a);

	    tessellator->color(outline);
	    tessellator->vertex(x + offset, y + outlineSize - halfBlend, 0.f);
	    tessellator->color(color);
	    tessellator->vertex(x + offset, y + outlineSize + halfBlend, 0.f);
	    tessellator->color(outline);
	    tessellator->vertex(x + width - offset, y + outlineSize - halfBlend, 0.f);

	    tessellator->color(color);
	    tessellator->vertex(x + offset, y + outlineSize + halfBlend, 0.f);
	    tessellator->vertex(x + width - offset, y + outlineSize + halfBlend, 0.f);
	    tessellator->color(outline);
	    tessellator->vertex(x + width - offset, y + outlineSize - halfBlend, 0.f);


	    tessellator->color(outline, 0.f);
	    tessellator->vertex(x + offset, y - blend, 0.f);
	    tessellator->color(outline);
	    tessellator->vertex(x + offset, y, 0.f);
	    tessellator->color(outline, 0.f);
	    tessellator->vertex(x + width - offset, y - blend, 0.f);

	    tessellator->color(outline);
	    tessellator->vertex(x + offset, y, 0.f);
	    tessellator->vertex(x + width - offset, y, 0.f);
	    tessellator->color(outline, 0.f);
	    tessellator->vertex(x + width - offset, y - blend, 0.f);
	}

	{ // Bottom
	    addFilledRectangle({x + offset, y + height - outlineSize + halfBlend, x + width - offset, y + height}, outline, outline.a);
	    addFilledRectangle({x + offset, y + height - offset, x + width - offset, y + height - outlineSize - halfBlend}, color, color.a);

	    tessellator->color(color);
	    tessellator->vertex(x + offset, y + height - outlineSize - halfBlend, 0.f);
	    tessellator->color(outline);
	    tessellator->vertex(x + offset, y + height - outlineSize + halfBlend, 0.f);
	    tessellator->color(color);
	    tessellator->vertex(x + width - offset, y + height - outlineSize - halfBlend, 0.f);

	    tessellator->color(outline);
	    tessellator->vertex(x + offset, y + height - outlineSize + halfBlend, 0.f);
	    tessellator->vertex(x + width - offset, y + height - outlineSize + halfBlend, 0.f);
	    tessellator->color(color);
	    tessellator->vertex(x + width - offset, y + height - outlineSize - halfBlend, 0.f);


	    tessellator->color(outline);
	    tessellator->vertex(x + offset, y + height, 0.f);
	    tessellator->color(outline, 0.f);
	    tessellator->vertex(x + offset, y + height + blend, 0.f);
	    tessellator->color(outline);
	    tessellator->vertex(x + width - offset, y + height, 0.f);

	    tessellator->color(outline, 0.f);
	    tessellator->vertex(x + offset, y + height + blend, 0.f);
	    tessellator->vertex(x + width - offset, y + height + blend, 0.f);
	    tessellator->color(outline);
	    tessellator->vertex(x + width - offset, y + height, 0.f);
	}

    // Middle
    addFilledRectangle({x + offset, y + offset, x + width - offset, y + height - offset}, color, color.a);

	if (cbr)
	    addOutlinedPartialCircleBlend({x + width - offset, y + height - offset}, 0.f, 90.f, stepSize, color, outline, outlineSize, radius, blend);
    else {
        addFilledRectangle(x + width - offset, y + height - offset, offset - outlineSize - halfBlend, offset - outlineSize - halfBlend, color, color.a);
        addFilledRectangle(x + width - outlineSize + halfBlend, y + height - offset, outlineSize - halfBlend, offset, outline, outline.a);
        addFilledRectangle(x + width - offset, y + height - outlineSize + halfBlend, offset - outlineSize + halfBlend, outlineSize - halfBlend, outline, outline.a);

        tessellator->color(color);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + height - offset);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + height - outlineSize - halfBlend);
        tessellator->color(outline);
        tessellator->vertex(x + width - outlineSize + halfBlend, y + height - outlineSize - halfBlend);

        tessellator->color(color);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + height - offset);
        tessellator->color(outline);
        tessellator->vertex(x + width - outlineSize + halfBlend, y + height - outlineSize - halfBlend);
        tessellator->vertex(x + width - outlineSize + halfBlend, y + height - offset);


        tessellator->color(color);
        tessellator->vertex(x + width - offset, y + height - outlineSize - halfBlend);
        tessellator->color(outline);
        tessellator->vertex(x + width - offset, y + height - outlineSize + halfBlend);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + height - outlineSize + halfBlend);

        tessellator->color(color);
        tessellator->vertex(x + width - offset, y + height - outlineSize - halfBlend);
        tessellator->color(outline);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + height - outlineSize + halfBlend);
        tessellator->color(color);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + height - outlineSize - halfBlend);


        tessellator->color(color);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + height - outlineSize - halfBlend);
        tessellator->color(outline);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + height - outlineSize + halfBlend);
        tessellator->vertex(x + width - outlineSize + halfBlend, y + height - outlineSize + halfBlend);

        tessellator->color(color);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + height - outlineSize - halfBlend);
        tessellator->color(outline);
        tessellator->vertex(x + width - outlineSize + halfBlend, y + height - outlineSize + halfBlend);
        tessellator->vertex(x + width - outlineSize + halfBlend, y + height - outlineSize - halfBlend);


        tessellator->color(outline);
        tessellator->vertex(x + width, y + height - offset);
        tessellator->vertex(x + width, y + height);
        tessellator->color(outline, 0.f);
        tessellator->vertex(x + width + blend, y + height);

        tessellator->color(outline);
        tessellator->vertex(x + width, y + height - offset);
        tessellator->color(outline, 0.f);
        tessellator->vertex(x + width + blend, y + height);
        tessellator->vertex(x + width + blend, y + height - offset);


        tessellator->color(outline);
        tessellator->vertex(x + width - offset, y + height);
        tessellator->color(outline, 0.f);
        tessellator->vertex(x + width - offset, y + height + blend);
        tessellator->vertex(x + width, y + height + blend);

        tessellator->color(outline);
        tessellator->vertex(x + width - offset, y + height);
        tessellator->color(outline, 0.f);
        tessellator->vertex(x + width, y + height + blend);
        tessellator->color(outline);
        tessellator->vertex(x + width, y + height);


        tessellator->color(outline);
        tessellator->vertex(x + width, y + height);
        tessellator->color(outline, 0.f);
        tessellator->vertex(x + width, y + height + blend);
        tessellator->vertex(x + width + blend, y + height + blend);

        tessellator->color(outline);
        tessellator->vertex(x + width, y + height);
        tessellator->color(outline, 0.f);
        tessellator->vertex(x + width + blend, y + height + blend);
        tessellator->vertex(x + width + blend, y + height);
    }

	if (cbl)
	    addOutlinedPartialCircleBlend({x + offset, y + height - offset}, 90.f, 180.f, stepSize, color, outline, outlineSize, radius, blend);
    else {
        addFilledRectangle(x + outlineSize + halfBlend, y + height - offset, offset - outlineSize - halfBlend, offset - outlineSize - halfBlend, color, color.a);
        addFilledRectangle(x, y + height - offset, outlineSize - halfBlend, offset, outline, outline.a);
        addFilledRectangle(x + outlineSize - halfBlend, y + height - outlineSize + halfBlend, offset - outlineSize + halfBlend, outlineSize - halfBlend, outline, outline.a);

        tessellator->color(outline);
        tessellator->vertex(x + outlineSize - halfBlend, y + height - offset);
        tessellator->vertex(x + outlineSize - halfBlend, y + height - outlineSize - halfBlend);
        tessellator->color(color);
        tessellator->vertex(x + outlineSize + halfBlend, y + height - outlineSize - halfBlend);

        tessellator->color(outline);
        tessellator->vertex(x + outlineSize - halfBlend, y + height - offset);
        tessellator->color(color);
        tessellator->vertex(x + outlineSize + halfBlend, y + height - outlineSize - halfBlend);
        tessellator->vertex(x + outlineSize + halfBlend, y + height - offset);


        tessellator->color(color);
        tessellator->vertex(x + outlineSize + halfBlend, y + height - outlineSize - halfBlend);
        tessellator->color(outline);
        tessellator->vertex(x + outlineSize + halfBlend, y + height - outlineSize + halfBlend);
        tessellator->vertex(x + offset, y + height - outlineSize + halfBlend);

        tessellator->color(color);
        tessellator->vertex(x + outlineSize + halfBlend, y + height - outlineSize - halfBlend);
        tessellator->color(outline);
        tessellator->vertex(x + offset, y + height - outlineSize + halfBlend);
        tessellator->color(color);
        tessellator->vertex(x + offset, y + height - outlineSize - halfBlend);


        tessellator->color(outline);
        tessellator->vertex(x + outlineSize - halfBlend, y + height - outlineSize - halfBlend);
        tessellator->vertex(x + outlineSize - halfBlend, y + height - outlineSize + halfBlend);
        tessellator->color(color);
        tessellator->vertex(x + outlineSize + halfBlend, y + height - outlineSize - halfBlend);

        tessellator->vertex(x + outlineSize + halfBlend, y + height - outlineSize - halfBlend);
        tessellator->color(outline);
        tessellator->vertex(x + outlineSize - halfBlend, y + height - outlineSize + halfBlend);
        tessellator->vertex(x + outlineSize + halfBlend, y + height - outlineSize + halfBlend);


        tessellator->color(outline, 0.f);
        tessellator->vertex(x - blend, y + height - offset);
        tessellator->vertex(x - blend, y + height);
        tessellator->color(outline);
        tessellator->vertex(x, y + height);

        tessellator->color(outline, 0.f);
        tessellator->vertex(x - blend, y + height - offset);
        tessellator->color(outline);
        tessellator->vertex(x, y + height);
        tessellator->vertex(x, y + height - offset);


        tessellator->vertex(x, y + height);
        tessellator->color(outline, 0.f);
        tessellator->vertex(x, y + height + blend);
        tessellator->vertex(x + offset, y + height + blend);

        tessellator->color(outline);
        tessellator->vertex(x, y + height);
        tessellator->color(outline, 0.f);
        tessellator->vertex(x + offset, y + height + blend);
        tessellator->color(outline);
        tessellator->vertex(x + offset, y + height);


        tessellator->color(outline, 0.f);
        tessellator->vertex(x - blend, y + height);
        tessellator->vertex(x - blend, y + height + blend);
        tessellator->color(outline);
        tessellator->vertex(x, y + height);

        tessellator->vertex(x, y + height);
        tessellator->color(outline, 0.f);
        tessellator->vertex(x - blend, y + height + blend);
        tessellator->vertex(x, y + height + blend);
    }

	if (ctl)
	    addOutlinedPartialCircleBlend({x + offset, y + offset}, 180.f, 270.f, stepSize, color, outline, outlineSize, radius, blend);
    else {
        addFilledRectangle(x + outlineSize + halfBlend, y + outlineSize + halfBlend, offset - outlineSize - halfBlend, offset - outlineSize - halfBlend, color, color.a);
        addFilledRectangle(x, y, outlineSize - halfBlend, offset, outline, outline.a);
        addFilledRectangle(x + outlineSize - halfBlend, y, offset - outlineSize + halfBlend, outlineSize - halfBlend, outline, outline.a);

        tessellator->color(outline);
        tessellator->vertex(x + outlineSize - halfBlend, y + outlineSize + halfBlend);
        tessellator->vertex(x + outlineSize - halfBlend, y + offset);
        tessellator->color(color);
        tessellator->vertex(x + outlineSize + halfBlend, y + offset);

        tessellator->color(outline);
        tessellator->vertex(x + outlineSize - halfBlend, y + outlineSize + halfBlend);
        tessellator->color(color);
        tessellator->vertex(x + outlineSize + halfBlend, y + offset);
        tessellator->vertex(x + outlineSize + halfBlend, y + outlineSize + halfBlend);


        tessellator->color(outline);
        tessellator->vertex(x + outlineSize + halfBlend, y + outlineSize - halfBlend);
        tessellator->color(color);
        tessellator->vertex(x + outlineSize + halfBlend, y + outlineSize + halfBlend);
        tessellator->vertex(x + offset, y + outlineSize + halfBlend);

        tessellator->color(outline);
        tessellator->vertex(x + outlineSize + halfBlend, y + outlineSize - halfBlend);
        tessellator->color(color);
        tessellator->vertex(x + offset, y + outlineSize + halfBlend);
        tessellator->color(outline);
        tessellator->vertex(x + offset, y + outlineSize - halfBlend);


        tessellator->vertex(x + outlineSize - halfBlend, y + outlineSize - halfBlend);
        tessellator->vertex(x + outlineSize - halfBlend, y + outlineSize + halfBlend);
        tessellator->color(color);
        tessellator->vertex(x + outlineSize + halfBlend, y + outlineSize + halfBlend);

        tessellator->color(outline);
        tessellator->vertex(x + outlineSize - halfBlend, y + outlineSize - halfBlend);
        tessellator->color(color);
        tessellator->vertex(x + outlineSize + halfBlend, y + outlineSize + halfBlend);
        tessellator->color(outline);
        tessellator->vertex(x + outlineSize + halfBlend, y + outlineSize - halfBlend);


        tessellator->color(outline, 0.f);
        tessellator->vertex(x - blend, y);
        tessellator->vertex(x - blend, y + offset);
        tessellator->color(outline);
        tessellator->vertex(x, y + offset);

        tessellator->color(outline, 0.f);
        tessellator->vertex(x - blend, y);
        tessellator->color(outline);
        tessellator->vertex(x, y + offset);
        tessellator->vertex(x, y);


        tessellator->color(outline, 0.f);
        tessellator->vertex(x, y - blend);
        tessellator->color(outline);
        tessellator->vertex(x, y);
        tessellator->vertex(x + offset, y);

        tessellator->color(outline, 0.f);
        tessellator->vertex(x, y - blend);
        tessellator->color(outline);
        tessellator->vertex(x + offset, y);
        tessellator->color(outline, 0.f);
        tessellator->vertex(x + offset, y - blend);


        tessellator->color(outline, 0.f);
        tessellator->vertex(x - blend, y - blend);
        tessellator->vertex(x - blend, y);
        tessellator->color(outline);
        tessellator->vertex(x, y);

        tessellator->color(outline, 0.f);
        tessellator->vertex(x - blend, y - blend);
        tessellator->color(outline);
        tessellator->vertex(x, y);
        tessellator->color(outline, 0.f);
        tessellator->vertex(x, y - blend);
    }

	if (ctr)
	    addOutlinedPartialCircleBlend({x + width - offset, y + offset}, 270.f, 360.f, stepSize, color, outline, outlineSize, radius, blend);
    else {
        addFilledRectangle(x + width - offset, y + outlineSize + halfBlend, offset - outlineSize - halfBlend, offset - outlineSize - halfBlend, color, color.a);
        addFilledRectangle(x + width - offset, y, offset, outlineSize - halfBlend, outline, outline.a);
        addFilledRectangle(x + width - outlineSize + halfBlend, y + outlineSize - halfBlend, outlineSize - halfBlend, offset - outlineSize + halfBlend, outline, outline.a);

        tessellator->color(outline);
        tessellator->vertex(x + width - offset, y + outlineSize - halfBlend);
        tessellator->color(color);
        tessellator->vertex(x + width - offset, y + outlineSize + halfBlend);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + outlineSize + halfBlend);

        tessellator->color(outline);
        tessellator->vertex(x + width - offset, y + outlineSize - halfBlend);
        tessellator->color(color);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + outlineSize + halfBlend);
        tessellator->color(outline);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + outlineSize - halfBlend);


        tessellator->color(color);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + outlineSize + halfBlend);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + offset);
        tessellator->color(outline);
        tessellator->vertex(x + width - outlineSize + halfBlend, y + offset);

        tessellator->color(color);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + outlineSize + halfBlend);
        tessellator->color(outline);
        tessellator->vertex(x + width - outlineSize + halfBlend, y + offset);
        tessellator->vertex(x + width - outlineSize + halfBlend, y + outlineSize + halfBlend);


        tessellator->color(outline);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + outlineSize - halfBlend);
        tessellator->color(color);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + outlineSize + halfBlend);
        tessellator->color(outline);
        tessellator->vertex(x + width - outlineSize + halfBlend, y + outlineSize - halfBlend);

        tessellator->vertex(x + width - outlineSize + halfBlend, y + outlineSize - halfBlend);
        tessellator->color(color);
        tessellator->vertex(x + width - outlineSize - halfBlend, y + outlineSize + halfBlend);
        tessellator->color(outline);
        tessellator->vertex(x + width - outlineSize + halfBlend, y + outlineSize + halfBlend);


        tessellator->color(outline, 0.f);
        tessellator->vertex(x + width - offset, y - blend);
        tessellator->color(outline);
        tessellator->vertex(x + width - offset, y);
        tessellator->vertex(x + width, y);

        tessellator->color(outline, 0.f);
        tessellator->vertex(x + width - offset, y - blend);
        tessellator->color(outline);
        tessellator->vertex(x + width, y);
        tessellator->color(outline, 0.f);
        tessellator->vertex(x + width, y - blend);


        tessellator->color(outline);
        tessellator->vertex(x + width, y);
        tessellator->vertex(x + width, y + offset);
        tessellator->color(outline, 0.f);
        tessellator->vertex(x + width + blend, y + offset);

        tessellator->color(outline);
        tessellator->vertex(x + width, y);
        tessellator->color(outline, 0.f);
        tessellator->vertex(x + width + blend, y + offset);
        tessellator->vertex(x + width + blend, y);


        tessellator->color(outline, 0.f);
        tessellator->vertex(x + width, y - blend);
        tessellator->color(outline);
        tessellator->vertex(x + width, y);
        tessellator->color(outline, 0.f);
        tessellator->vertex(x + width + blend, y - blend);

        tessellator->vertex(x + width + blend, y - blend);
        tessellator->color(outline);
        tessellator->vertex(x + width, y);
        tessellator->color(outline, 0.f);
        tessellator->vertex(x + width + blend, y);
    }
}

void DrawUtils::addRoundedOutlinedRectangleBlendChroma(const float x, const float y, const float width, const float height, const float stepSize,
    const float alpha, const float outlineAlpha, const float outlineSize, const uint8_t flags, const float radius, const float blend) {
    if (tessellator == nullptr)
        return;

    if (blend == 0.f) {
        addRoundedOutlinedRectangle(x, y, width, height, stepSize, {1.f, 1.f, 1.f, alpha}, {1.f, 1.f, 1.f, outlineAlpha}, outlineSize, flags, radius);
        return;
    }

    const bool ctl = flags & 8; // Top left corner, renders rounded if true
    const bool ctr = flags & 4; // Top right corner, renders rounded if true
    const bool cbl = flags & 2; // Bottom left corner, renders rounded if true
    const bool cbr = flags & 1; // Bottom right corner, renders rounded if true

	const float offset = std::min(std::min(radius, width / 2.f), height / 2.f);
    const float halfBlend = blend / 2.f;

	{ // Left
	    addFilledRectangleChroma({x, y + offset, x + outlineSize - halfBlend, y + height - offset}, outlineAlpha);
	    addFilledRectangleChroma({x + outlineSize + halfBlend, y + offset, x + offset, y + height - offset}, alpha);

	    tessellator->vertexChroma(x + outlineSize - halfBlend, y + offset, outlineAlpha);
	    tessellator->vertexChroma(x + outlineSize - halfBlend, y + height - offset, outlineAlpha);
	    tessellator->vertexChroma(x + outlineSize + halfBlend, y + offset, alpha);

	    tessellator->vertexChroma(x + outlineSize - halfBlend, y + height - offset, outlineAlpha);
	    tessellator->vertexChroma(x + outlineSize + halfBlend, y + height - offset, alpha);
	    tessellator->vertexChroma(x + outlineSize + halfBlend, y + offset, alpha);


	    tessellator->vertexChroma(x - blend, y + offset, 0.f);
	    tessellator->vertexChroma(x - blend, y + height - offset, 0.f);
	    tessellator->vertexChroma(x, y + offset, outlineAlpha);

	    tessellator->vertexChroma(x - blend, y + height - offset, 0.f);
	    tessellator->vertexChroma(x, y + height - offset, outlineAlpha);
	    tessellator->vertexChroma(x, y + offset, outlineAlpha);
	}

    { // Right
        addFilledRectangleChroma({x + width - outlineSize + halfBlend, y + offset, x + width, y + height - offset}, outlineAlpha);
        addFilledRectangleChroma({x + width - offset, y + offset, x + width - outlineSize - halfBlend, y + height - offset}, alpha);

        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + offset, alpha);
        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + height - offset, alpha);
        tessellator->vertexChroma(x + width - outlineSize + halfBlend, y + offset, outlineAlpha);

        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + height - offset, alpha);
        tessellator->vertexChroma(x + width - outlineSize + halfBlend, y + height - offset, outlineAlpha);
        tessellator->vertexChroma(x + width - outlineSize + halfBlend, y + offset, outlineAlpha);

        tessellator->vertexChroma(x + width, y + offset, outlineAlpha);
        tessellator->vertexChroma(x + width, y + height - offset, outlineAlpha);
        tessellator->vertexChroma(x + width + blend, y + offset, 0.f);

        tessellator->vertexChroma(x + width, y + height - offset, outlineAlpha);
        tessellator->vertexChroma(x + width + blend, y + height - offset, 0.f);
        tessellator->vertexChroma(x + width + blend, y + offset, 0.f);
    }

	{ // Top
	    addFilledRectangleChroma({x + offset, y, x + width - offset, y + outlineSize - halfBlend}, outlineAlpha);
	    addFilledRectangleChroma({x + offset, y + outlineSize + halfBlend, x + width - offset, y + offset}, alpha);

	    tessellator->vertexChroma(x + offset, y + outlineSize - halfBlend, outlineAlpha);
	    tessellator->vertexChroma(x + offset, y + outlineSize + halfBlend, alpha);
	    tessellator->vertexChroma(x + width - offset, y + outlineSize - halfBlend, outlineAlpha);

	    tessellator->vertexChroma(x + offset, y + outlineSize + halfBlend, alpha);
	    tessellator->vertexChroma(x + width - offset, y + outlineSize + halfBlend, alpha);
	    tessellator->vertexChroma(x + width - offset, y + outlineSize - halfBlend, outlineAlpha);


	    tessellator->vertexChroma(x + offset, y - blend, 0.f);
	    tessellator->vertexChroma(x + offset, y, outlineAlpha);
	    tessellator->vertexChroma(x + width - offset, y - blend, 0.f);

	    tessellator->vertexChroma(x + offset, y, outlineAlpha);
	    tessellator->vertexChroma(x + width - offset, y, outlineAlpha);
	    tessellator->vertexChroma(x + width - offset, y - blend, 0.f);
	}

	{ // Bottom
	    addFilledRectangleChroma({x + offset, y + height - outlineSize + halfBlend, x + width - offset, y + height}, outlineAlpha);
	    addFilledRectangleChroma({x + offset, y + height - offset, x + width - offset, y + height - outlineSize - halfBlend}, alpha);

	    tessellator->vertexChroma(x + offset, y + height - outlineSize - halfBlend, alpha);
	    tessellator->vertexChroma(x + offset, y + height - outlineSize + halfBlend, outlineAlpha);
	    tessellator->vertexChroma(x + width - offset, y + height - outlineSize - halfBlend, alpha);

	    tessellator->vertexChroma(x + offset, y + height - outlineSize + halfBlend, outlineAlpha);
	    tessellator->vertexChroma(x + width - offset, y + height - outlineSize + halfBlend, outlineAlpha);
	    tessellator->vertexChroma(x + width - offset, y + height - outlineSize - halfBlend, alpha);


	    tessellator->vertexChroma(x + offset, y + height, outlineAlpha);
	    tessellator->vertexChroma(x + offset, y + height + blend, 0.f);
	    tessellator->vertexChroma(x + width - offset, y + height, outlineAlpha);

	    tessellator->vertexChroma(x + offset, y + height + blend, 0.f);
	    tessellator->vertexChroma(x + width - offset, y + height + blend, 0.f);
	    tessellator->vertexChroma(x + width - offset, y + height, outlineAlpha);
	}

    // Middle
    addFilledRectangleChroma({x + offset, y + offset, x + width - offset, y + height - offset}, alpha);

	if (cbr)
	    addOutlinedPartialCircleBlendChroma({x + width - offset, y + height - offset}, 0.f, 90.f, stepSize, alpha, outlineAlpha, outlineSize, radius, blend);
    else {
        addFilledRectangleChroma(x + width - offset, y + height - offset, offset - outlineSize - halfBlend, offset - outlineSize - halfBlend, alpha);
        addFilledRectangleChroma(x + width - outlineSize + halfBlend, y + height - offset, outlineSize - halfBlend, offset, outlineAlpha);
        addFilledRectangleChroma(x + width - offset, y + height - outlineSize + halfBlend, offset - outlineSize + halfBlend, outlineSize - halfBlend, outlineAlpha);

        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + height - offset, alpha);
        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + height - outlineSize - halfBlend, alpha);
        tessellator->vertexChroma(x + width - outlineSize + halfBlend, y + height - outlineSize - halfBlend, outlineAlpha);

        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + height - offset, alpha);
        tessellator->vertexChroma(x + width - outlineSize + halfBlend, y + height - outlineSize - halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + width - outlineSize + halfBlend, y + height - offset, outlineAlpha);


        tessellator->vertexChroma(x + width - offset, y + height - outlineSize - halfBlend, alpha);
        tessellator->vertexChroma(x + width - offset, y + height - outlineSize + halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + height - outlineSize + halfBlend, outlineAlpha);

        tessellator->vertexChroma(x + width - offset, y + height - outlineSize - halfBlend, alpha);
        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + height - outlineSize + halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + height - outlineSize - halfBlend, alpha);


        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + height - outlineSize - halfBlend, alpha);
        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + height - outlineSize + halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + width - outlineSize + halfBlend, y + height - outlineSize + halfBlend, outlineAlpha);

        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + height - outlineSize - halfBlend, alpha);
        tessellator->vertexChroma(x + width - outlineSize + halfBlend, y + height - outlineSize + halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + width - outlineSize + halfBlend, y + height - outlineSize - halfBlend, outlineAlpha);


        tessellator->vertexChroma(x + width, y + height - offset, outlineAlpha);
        tessellator->vertexChroma(x + width, y + height, outlineAlpha);
        tessellator->vertexChroma(x + width + blend, y + height, 0.f);

        tessellator->vertexChroma(x + width, y + height - offset, outlineAlpha);
        tessellator->vertexChroma(x + width + blend, y + height, 0.f);
        tessellator->vertexChroma(x + width + blend, y + height - offset, 0.f);


        tessellator->vertexChroma(x + width - offset, y + height, outlineAlpha);
        tessellator->vertexChroma(x + width - offset, y + height + blend, 0.f);
        tessellator->vertexChroma(x + width, y + height + blend, 0.f);

        tessellator->vertexChroma(x + width - offset, y + height, outlineAlpha);
        tessellator->vertexChroma(x + width, y + height + blend, 0.f);
        tessellator->vertexChroma(x + width, y + height, outlineAlpha);


        tessellator->vertexChroma(x + width, y + height, outlineAlpha);
        tessellator->vertexChroma(x + width, y + height + blend, 0.f);
        tessellator->vertexChroma(x + width + blend, y + height + blend, 0.f);

        tessellator->vertexChroma(x + width, y + height, outlineAlpha);
        tessellator->vertexChroma(x + width + blend, y + height + blend, 0.f);
        tessellator->vertexChroma(x + width + blend, y + height, 0.f);
    }

	if (cbl)
	    addOutlinedPartialCircleBlendChroma({x + offset, y + height - offset}, 90.f, 180.f, stepSize, alpha, outlineAlpha, outlineSize, radius, blend);
    else {
        addFilledRectangleChroma(x + outlineSize + halfBlend, y + height - offset, offset - outlineSize - halfBlend, offset - outlineSize - halfBlend, alpha);
        addFilledRectangleChroma(x, y + height - offset, outlineSize - halfBlend, offset, outlineAlpha);
        addFilledRectangleChroma(x + outlineSize - halfBlend, y + height - outlineSize + halfBlend, offset - outlineSize + halfBlend, outlineSize - halfBlend, outlineAlpha);

        tessellator->vertexChroma(x + outlineSize - halfBlend, y + height - offset, outlineAlpha);
        tessellator->vertexChroma(x + outlineSize - halfBlend, y + height - outlineSize - halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + outlineSize + halfBlend, y + height - outlineSize - halfBlend, alpha);

        tessellator->vertexChroma(x + outlineSize - halfBlend, y + height - offset, outlineAlpha);
        tessellator->vertexChroma(x + outlineSize + halfBlend, y + height - outlineSize - halfBlend, alpha);
        tessellator->vertexChroma(x + outlineSize + halfBlend, y + height - offset, alpha);


        tessellator->vertexChroma(x + outlineSize + halfBlend, y + height - outlineSize - halfBlend, alpha);
        tessellator->vertexChroma(x + outlineSize + halfBlend, y + height - outlineSize + halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + offset, y + height - outlineSize + halfBlend, outlineAlpha);

        tessellator->vertexChroma(x + outlineSize + halfBlend, y + height - outlineSize - halfBlend, alpha);
        tessellator->vertexChroma(x + offset, y + height - outlineSize + halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + offset, y + height - outlineSize - halfBlend, alpha);


        tessellator->vertexChroma(x + outlineSize - halfBlend, y + height - outlineSize - halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + outlineSize - halfBlend, y + height - outlineSize + halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + outlineSize + halfBlend, y + height - outlineSize - halfBlend, alpha);

        tessellator->vertexChroma(x + outlineSize + halfBlend, y + height - outlineSize - halfBlend, alpha);
        tessellator->vertexChroma(x + outlineSize - halfBlend, y + height - outlineSize + halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + outlineSize + halfBlend, y + height - outlineSize + halfBlend, outlineAlpha);


        tessellator->vertexChroma(x - blend, y + height - offset, 0.f);
        tessellator->vertexChroma(x - blend, y + height, 0.f);
        tessellator->vertexChroma(x, y + height, outlineAlpha);

        tessellator->vertexChroma(x - blend, y + height - offset, 0.f);
        tessellator->vertexChroma(x, y + height, outlineAlpha);
        tessellator->vertexChroma(x, y + height - offset, outlineAlpha);


        tessellator->vertexChroma(x, y + height, outlineAlpha);
        tessellator->vertexChroma(x, y + height + blend, 0.f);
        tessellator->vertexChroma(x + offset, y + height + blend);

        tessellator->vertexChroma(x, y + height, outlineAlpha);
        tessellator->vertexChroma(x + offset, y + height + blend, 0.f);
        tessellator->vertexChroma(x + offset, y + height, outlineAlpha);


        tessellator->vertexChroma(x - blend, y + height, 0.f);
        tessellator->vertexChroma(x - blend, y + height + blend, 0.f);
        tessellator->vertexChroma(x, y + height, outlineAlpha);

        tessellator->vertexChroma(x, y + height, outlineAlpha);
        tessellator->vertexChroma(x - blend, y + height + blend, 0.f);
        tessellator->vertexChroma(x, y + height + blend, 0.f);
    }

	if (ctl)
	    addOutlinedPartialCircleBlendChroma({x + offset, y + offset}, 180.f, 270.f, stepSize, alpha, outlineAlpha, outlineSize, radius, blend);
    else {
        addFilledRectangleChroma(x + outlineSize + halfBlend, y + outlineSize + halfBlend, offset - outlineSize - halfBlend, offset - outlineSize - halfBlend, alpha);
        addFilledRectangleChroma(x, y, outlineSize - halfBlend, offset, outlineAlpha);
        addFilledRectangleChroma(x + outlineSize - halfBlend, y, offset - outlineSize + halfBlend, outlineSize - halfBlend, outlineAlpha);

        tessellator->vertexChroma(x + outlineSize - halfBlend, y + outlineSize + halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + outlineSize - halfBlend, y + offset, outlineAlpha);
        tessellator->vertexChroma(x + outlineSize + halfBlend, y + offset, alpha);

        tessellator->vertexChroma(x + outlineSize - halfBlend, y + outlineSize + halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + outlineSize + halfBlend, y + offset, alpha);
        tessellator->vertexChroma(x + outlineSize + halfBlend, y + outlineSize + halfBlend, alpha);


        tessellator->vertexChroma(x + outlineSize + halfBlend, y + outlineSize - halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + outlineSize + halfBlend, y + outlineSize + halfBlend, alpha);
        tessellator->vertexChroma(x + offset, y + outlineSize + halfBlend, alpha);

        tessellator->vertexChroma(x + outlineSize + halfBlend, y + outlineSize - halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + offset, y + outlineSize + halfBlend, alpha);
        tessellator->vertexChroma(x + offset, y + outlineSize - halfBlend, outlineAlpha);


        tessellator->vertexChroma(x + outlineSize - halfBlend, y + outlineSize - halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + outlineSize - halfBlend, y + outlineSize + halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + outlineSize + halfBlend, y + outlineSize + halfBlend, alpha);

        tessellator->vertexChroma(x + outlineSize - halfBlend, y + outlineSize - halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + outlineSize + halfBlend, y + outlineSize + halfBlend, alpha);
        tessellator->vertexChroma(x + outlineSize + halfBlend, y + outlineSize - halfBlend, outlineAlpha);


        tessellator->vertexChroma(x - blend, y, 0.f);
        tessellator->vertexChroma(x - blend, y + offset, 0.f);
        tessellator->vertexChroma(x, y + offset, outlineAlpha);

        tessellator->vertexChroma(x - blend, y, 0.f);
        tessellator->vertexChroma(x, y + offset, outlineAlpha);
        tessellator->vertexChroma(x, y, outlineAlpha);


        tessellator->vertexChroma(x, y - blend, 0.f);
        tessellator->vertexChroma(x, y, outlineAlpha);
        tessellator->vertexChroma(x + offset, y, outlineAlpha);

        tessellator->vertexChroma(x, y - blend, 0.f);
        tessellator->vertexChroma(x + offset, y, outlineAlpha);
        tessellator->vertexChroma(x + offset, y - blend, 0.f);


        tessellator->vertexChroma(x - blend, y - blend, 0.f);
        tessellator->vertexChroma(x - blend, y, 0.f);
        tessellator->vertexChroma(x, y, outlineAlpha);

        tessellator->vertexChroma(x - blend, y - blend, 0.f);
        tessellator->vertexChroma(x, y, outlineAlpha);
        tessellator->vertexChroma(x, y - blend, 0.f);
    }

	if (ctr)
	    addOutlinedPartialCircleBlendChroma({x + width - offset, y + offset}, 270.f, 360.f, stepSize, alpha, outlineAlpha, outlineSize, radius, blend);
    else {
        addFilledRectangleChroma(x + width - offset, y + outlineSize + halfBlend, offset - outlineSize - halfBlend, offset - outlineSize - halfBlend, alpha);
        addFilledRectangleChroma(x + width - offset, y, offset, outlineSize - halfBlend, outlineAlpha);
        addFilledRectangleChroma(x + width - outlineSize + halfBlend, y + outlineSize - halfBlend, outlineSize - halfBlend, offset - outlineSize + halfBlend, outlineAlpha);

        tessellator->vertexChroma(x + width - offset, y + outlineSize - halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + width - offset, y + outlineSize + halfBlend, alpha);
        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + outlineSize + halfBlend, alpha);

        tessellator->vertexChroma(x + width - offset, y + outlineSize - halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + outlineSize + halfBlend, alpha);
        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + outlineSize - halfBlend, outlineAlpha);


        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + outlineSize + halfBlend, alpha);
        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + offset, alpha);
        tessellator->vertexChroma(x + width - outlineSize + halfBlend, y + offset, outlineAlpha);

        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + outlineSize + halfBlend, alpha);
        tessellator->vertexChroma(x + width - outlineSize + halfBlend, y + offset, outlineAlpha);
        tessellator->vertexChroma(x + width - outlineSize + halfBlend, y + outlineSize + halfBlend, outlineAlpha);


        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + outlineSize - halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + outlineSize + halfBlend, alpha);
        tessellator->vertexChroma(x + width - outlineSize + halfBlend, y + outlineSize - halfBlend, outlineAlpha);

        tessellator->vertexChroma(x + width - outlineSize + halfBlend, y + outlineSize - halfBlend, outlineAlpha);
        tessellator->vertexChroma(x + width - outlineSize - halfBlend, y + outlineSize + halfBlend, alpha);
        tessellator->vertexChroma(x + width - outlineSize + halfBlend, y + outlineSize + halfBlend, outlineAlpha);


        tessellator->vertexChroma(x + width - offset, y - blend, 0.f);
        tessellator->vertexChroma(x + width - offset, y, outlineAlpha);
        tessellator->vertexChroma(x + width, y, outlineAlpha);

        tessellator->vertexChroma(x + width - offset, y - blend, 0.f);
        tessellator->vertexChroma(x + width, y, outlineAlpha);
        tessellator->vertexChroma(x + width, y - blend, 0.f);


        tessellator->vertexChroma(x + width, y, outlineAlpha);
        tessellator->vertexChroma(x + width, y + offset, outlineAlpha);
        tessellator->vertexChroma(x + width + blend, y + offset, 0.f);

        tessellator->vertexChroma(x + width, y, outlineAlpha);
        tessellator->vertexChroma(x + width + blend, y + offset, 0.f);
        tessellator->vertexChroma(x + width + blend, y, 0.f);


        tessellator->vertexChroma(x + width, y - blend, 0.f);
        tessellator->vertexChroma(x + width, y, outlineAlpha);
        tessellator->vertexChroma(x + width + blend, y - blend, 0.f);

        tessellator->vertexChroma(x + width + blend, y - blend, 0.f);
        tessellator->vertexChroma(x + width, y, outlineAlpha);
        tessellator->vertexChroma(x + width + blend, y, 0.f);
    }
}
