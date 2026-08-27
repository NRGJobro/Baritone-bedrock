#pragma once

#include "../Core/SemVersion.h"

struct BaseGameVersion {
    SemVersion semVersion;
    bool neverCompatible;
};
