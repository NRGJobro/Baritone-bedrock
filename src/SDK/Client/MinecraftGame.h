#pragma once

#include "ClientInstance.h"
#include "Font/FontRepository.h"
#include "Font/Fonts.h"

class MinecraftGame {
public:
    struct CIHolder {
        void* opaque;
        std::shared_ptr<ClientInstance> clientInstance;
    };

    ClientInstance* getClientInstance();
    FontRepository* getFontRepository();

    Font* getFont(Fonts font);
};
