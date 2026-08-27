#pragma once

#include "P2P/NetworkID.h"
#include "Realms/NetworkID.h"

namespace NetherNet {
    struct NetworkID : std::variant<std::monostate, P2P::NetworkID, Realms::NetworkID> {};
}
