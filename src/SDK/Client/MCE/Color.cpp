#include "Color.h"

mce::Color::Color() {
    this->r = 1.f;
    this->g = 1.f;
    this->b = 1.f;
    this->a = 1.f;
}

mce::Color::Color(const int color) {
    this->r = static_cast<float>((color >> 16) & 0xFF) / 255.f;
    this->g = static_cast<float>((color >> 8) & 0xFF) / 255.f;
    this->b = static_cast<float>(color & 0xFF) / 255.f;
    this->a = static_cast<float>((color >> 24) & 0xFF) / 255.f;
}

mce::Color::Color(const float r, const float g, const float b, const float a) {
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
}

mce::Color::Color(const int r, const int g, const int b, const int a) {
    this->r = static_cast<float>(r) / 255.f;
    this->g = static_cast<float>(g) / 255.f;
    this->b = static_cast<float>(b) / 255.f;
    this->a = static_cast<float>(a) / 255.f;
}

mce::Color::Color(const Color& color, const float alpha) {
    this->r = color.r;
    this->g = color.g;
    this->b = color.b;
    this->a = alpha;
}

mce::Color mce::Color::operator+(const Color& other) const {
    return {this->r + other.r, this->g + other.g, this->b + other.b, this->a + other.a};
}

mce::Color mce::Color::operator*(const Color& other) const {
    return {this->r * other.r, this->g * other.g, this->b * other.b, this->a * other.a};
}

bool mce::Color::operator!=(const Color& other) const {
    return this->r != other.r || this->g != other.g || this->b != other.b;
}

mce::Color mce::Color::mix(const Color& other, const float percent) const {
    const float clampedPercent = std::clamp(percent, 0.f, 1.f);

    // Linear interpolation for each color component
    const float mixedR = this->r + (other.r - this->r) * clampedPercent;
    const float mixedG = this->g + (other.g - this->g) * clampedPercent;
    const float mixedB = this->b + (other.b - this->b) * clampedPercent;
    const float mixedA = this->a + (other.a - this->a) * clampedPercent;

    return {mixedR, mixedG, mixedB, mixedA};
}

int mce::Color::toARGB() const {
    return (static_cast<int>(this->r * 255.f) & 0xFF) << 16 | (static_cast<int>(this->g * 255.f) & 0xFF) << 8 | static_cast<int>(this->b * 255.f) & 0xFF | (static_cast<int>(this->a * 255.f) & 0xFF) << 24;
}
