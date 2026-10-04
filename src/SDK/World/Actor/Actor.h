#pragma once

#include "../Level/Level.h"
#include "Components/StateVectorComponent.h"
#include "Components/ActorRotationComponent.h"
#include "Components/ActorHeadRotationComponent.h"
#include "Components/MobBodyRotationComponent.h"
#include "Components/MovementFlags.h"
#include "EntityContext/EntityContext.h"

#include <cstdint>

// Source used by Bedrock's native arm-swing animation. Mine is the same
// first-person animation triggered by a normal pickaxe/block break.
enum class ActorSwingSource : std::uint8_t {
    None,
    Build,
    Mine,
    Interact,
    Attack,
    UseItem,
    ThrowItem,
    DropItem,
    Event
};

class Actor {
public:
    EntityContext& getEntityContext() const;

    Level* getLevel();
    [[nodiscard]] glm::vec3 getPosition();
    [[nodiscard]] glm::vec3 getFeetPosition();
    [[nodiscard]] glm::vec2 getRotation();
    void setRotation(const glm::vec2& rotation);
    [[nodiscard]] bool isOnGround() const;
    [[nodiscard]] bool isInWater() const;
    void jumpFromGround() const;
    void swing();

    template <typename T>
    auto view() const {
        return this->getEntityContext().enttRegistry.view<T>();
    }

    template <typename T>
    [[nodiscard]] bool hasComponent() const {
        return this->getEntityContext().enttRegistry.all_of<T>(this->getEntityContext().entity);
    }

    template <typename T>
    void addComponent() const {
        if (!this->hasComponent<T>())
            this->getEntityContext().enttRegistry.emplace<T>(this->getEntityContext().entity);
    }

    template<typename T>
    void removeComponent() const {
        if (this->hasComponent<T>())
            this->getEntityContext().enttRegistry.remove<T>(this->getEntityContext().entity);
    }

    template<typename T>
    T* tryGet() {
        return this->getEntityContext().enttRegistry.try_get<T>(this->getEntityContext().entity);
    }

    template <>
    StateVectorComponent* tryGet();
};
