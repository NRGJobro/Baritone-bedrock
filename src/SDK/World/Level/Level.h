#pragma once

#include "HitResult/HitResultWrapper.h"
#include "LevelData.h"
#include "Storage/GameRules.h"

class Level {
public:
    LevelData* getLevelData();
    HitResultWrapper* getHitResultWrapper();
    const std::string& getLevelId();
    std::weak_ptr<EntityRegistry>& getLevelEntity();
    GameRules* getGameRules();

    const HitResult& getHitResult();
    const HitResult& getLiquidHitResult();
};
