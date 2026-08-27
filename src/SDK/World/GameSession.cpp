#include "GameSession.h"

#include "Actor/Components/LevelComponent.h"

Level* GameSession::getLevel() const {
    return this->context.enttRegistry.try_get<LevelComponent>(this->context.entity)->level;
}
