#pragma once

#include "../Client/MCE/TexturePtr.h"

struct ItemGraphics {
    std::weak_ptr<void*> renderer;
    mce::TexturePtr atlasTexture;
};
