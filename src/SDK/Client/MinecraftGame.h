#pragma once

#include "ClientInstance.h"
#include "Font/FontRepository.h"
#include "Font/Fonts.h"
#include "MCE/TextureGroup.h"

class MinecraftGame {
public:
    struct CIHolder {
        void* opaque;
        std::shared_ptr<ClientInstance> clientInstance;
    };

    ClientInstance* getClientInstance();
    std::shared_ptr<mce::TextureGroup> getTextureGroup();
    FontRepository* getFontRepository();

    void grabMouse();
    void releaseMouse();

    Font* getFont(Fonts font);
};
