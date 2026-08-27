#pragma once

namespace mce {
    class Color {
    public:
        float r, g, b, a;

        Color();
        explicit Color(int color);
        Color(float r, float g, float b, float a = 1.f);
        Color(int r, int g, int b, int a = 255);
        Color(const Color& color, float alpha);

        [[nodiscard]] Color mix(const Color& other, float percent) const;

        [[nodiscard]] int toARGB() const;

        Color operator+(const Color& other) const;
        Color operator*(const Color& other) const;
        bool operator!=(const Color& other) const;
    };
}
