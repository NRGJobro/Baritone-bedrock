#pragma once

struct RectangleArea {
    float minX;
    float maxX;
    float minY;
    float maxY;

    RectangleArea() = default;
    RectangleArea(const glm::vec4& pos) : minX(pos.x), maxX(pos.z), minY(pos.y), maxY(pos.w) {}
};
