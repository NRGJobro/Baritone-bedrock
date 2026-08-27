#pragma once

#include "ClientResourcePointer.h"
#include "Texture.h"
#include "../Dragon/ClientTexture.h"

namespace mce {
    struct ClientTexture : ClientResourcePointer<std::variant<std::monostate, Texture, dragon::res::ClientTexture>> { };
}
