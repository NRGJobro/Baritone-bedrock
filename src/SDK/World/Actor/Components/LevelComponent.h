#pragma once

#include "IEntityComponent.h"

class LevelComponent : public IEntityComponent {
public:
    class Level* level;
};
