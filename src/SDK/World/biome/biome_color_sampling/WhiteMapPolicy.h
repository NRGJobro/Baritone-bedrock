#pragma once

#include "MapPolicy.h"

class WhiteMapPolicy : public MapPolicy {
public:
    mce::Color get(BlockSource* region, const glm::ivec3& pos) override;
};
