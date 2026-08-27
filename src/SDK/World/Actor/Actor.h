#pragma once

#include "../Dimension.h"
#include "../Level/Level.h"
#include "ActorLocation.h"
#include "Components/StateVectorComponent.h"
#include "Components/ActorRotationComponent.h"
#include "Components/MovementFlags.h"
#include "EntityContext/EntityContext.h"

class Actor {
public:
    EntityContext& getEntityContext() const;

    Dimension* getDimension();
    Level* getLevel();

    glm::vec3 getAttachPos(ActorLocation location, float a);
    [[nodiscard]] glm::vec3 getPosition();
    [[nodiscard]] glm::vec3 getFeetPosition();
    [[nodiscard]] glm::vec2 getRotation();
    void setRotation(const glm::vec2& rotation);
    [[nodiscard]] bool isOnGround() const;
    [[nodiscard]] bool isInWater() const;
    void jumpFromGround() const;

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
