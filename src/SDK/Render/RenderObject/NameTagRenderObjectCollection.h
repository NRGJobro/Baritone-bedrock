#pragma once

struct NameTagRenderObjectCollection {
    std::vector<NameTagRenderObject, LinearAllocator<NameTagRenderObject>> nameTags;
};
