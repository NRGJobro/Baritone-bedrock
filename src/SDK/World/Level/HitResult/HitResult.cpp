#include "HitResult.h"

#include "../../Actor/Components/ActorOwnerComponent.h"

Actor* HitResult::getEntity() const {
    if (this->type != HitResultType::Entity)
        return nullptr;

    const auto registry = this->entity.registry.handle.lock();

    if (registry == nullptr)
        return nullptr;

    const auto& owned = registry->ownedRegistry;
    const auto comp = owned.try_get<ActorOwnerComponent>(this->entity.entity);

    return comp != nullptr ? comp->entity.get() : nullptr;
}
