#include "Level.h"

#include "../../../Utils/Utils.h"

LevelData* Level::getLevelData() {
    return hat::member_at<LevelData*>(this, 0x90);
}

HitResultWrapper* Level::getHitResultWrapper() {
    return hat::member_at<HitResultWrapper*>(this, 0x1E8);
}

const std::string& Level::getLevelId() {
    return hat::member_at<std::string>(this, 0x258);
}
std::weak_ptr<EntityRegistry>& Level::getLevelEntity() {
    return hat::member_at<std::weak_ptr<EntityRegistry>>(this, 0x320);
}

GameRules* Level::getGameRules() {
    return Utils::CallVFunc<334, GameRules*>(this);
}

const HitResult& Level::getHitResult() {
    return this->getHitResultWrapper()->hitResult;
}

const HitResult& Level::getLiquidHitResult() {
    return this->getHitResultWrapper()->liquidHitResult;
}
