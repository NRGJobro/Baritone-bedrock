#pragma once

#include "../../../Client/MCE/Color.h"

class MapPolicy {
public:
    virtual ~MapPolicy() = default;
    virtual mce::Color get(class BlockSource* region, const glm::ivec3& pos);
};
