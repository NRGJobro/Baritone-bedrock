#include "Actor.h"

#include "../../../Memory/Sig/SignatureManager.h"
#include "../../../Utils/Utils.h"

EntityContext& Actor::getEntityContext() const {
    return hat::member_at<EntityContext>(const_cast<Actor*>(this), 0x8);
}

Dimension* Actor::getDimension() {
    return hat::member_at<Dimension*>(this, 0x1C8);
}

Level* Actor::getLevel() {
    return hat::member_at<Level*>(this, 0x1D8);
}

glm::vec3 Actor::getAttachPos(const ActorLocation location, const float a) {
    static auto sig = GET_SIG("Actor::getAttachPos");
    static auto getAttachPos = *(decltype(&Actor::getAttachPos)*)&sig;
    return (this->*getAttachPos)(location, a);
}

glm::vec3 Actor::getPosition() {
    const auto state = this->tryGet<StateVectorComponent>();
    return state == nullptr ? glm::vec3{} : state->pos;
}

glm::vec3 Actor::getFeetPosition() {
    return this->getPosition() - glm::vec3{0.f, 1.62f, 0.f};
}

glm::vec2 Actor::getRotation() {
    const auto rotation = this->tryGet<ActorRotationComponent>();
    return rotation == nullptr ? glm::vec2{} : rotation->rotation;
}

void Actor::setRotation(const glm::vec2& value) {
    if (const auto rotation = this->tryGet<ActorRotationComponent>()) {
        // Keep Bedrock's interpolation endpoints together when automation turns
        // the player. Updating only the current value makes the renderer blend
        // back toward a stale yaw every frame, which appears as rotation jitter.
        rotation->previousRotation = value;
        rotation->rotation = value;
    }
}

bool Actor::isOnGround() const {
    return this->hasComponent<OnGroundFlagComponent>();
}

bool Actor::isInWater() const {
    return this->hasComponent<WasInWaterFlagComponent>();
}

void Actor::jumpFromGround() const {
    this->addComponent<JumpFromGroundRequestComponent>();
}

void Actor::swing() {
    // Actor::swing is a native client-side animation entry point. Calling it
    // alongside GameMode's destroy lifecycle keeps the first-person hand and
    // remote arm animation in sync with automated mining.
    Utils::CallVFunc<110, void, ActorSwingSource>(this, ActorSwingSource::Mine);
}

template <>
StateVectorComponent* Actor::tryGet() {
    return hat::member_at<StateVectorComponent*>(this, 0x218);
}
