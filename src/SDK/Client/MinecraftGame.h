#pragma once

#include "ClientInstance.h"
#include "Font/FontRepository.h"
#include "Font/Fonts.h"
#include "MCE/TextureGroup.h"
#include "../Server/ServerInstance.h"

class MinecraftGame {
public:
    ClientInstance* getClientInstance();
    std::shared_ptr<mce::TextureGroup> getTextureGroup();
    std::shared_ptr<FontRepository> getFontRepository();
    ServerInstance* getServerInstance();

    void grabMouse();
    void releaseMouse();

    Font* getFont(Fonts font);
};
