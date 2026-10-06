#pragma once

class ClientInstance;
class FontRepository;
class Font;
enum class Fonts;

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
