#pragma once

#include "../Components/IEntityComponent.h"

struct EntityId {
    std::uint32_t rawId;

    [[nodiscard]] constexpr bool operator==(const EntityId& other) const = default;

    [[nodiscard]] constexpr operator std::uint32_t() const {
        return this->rawId;
    }
};

struct EntityIdTraits {
	using value_type = EntityId;

	using entity_type = uint32_t;
	using version_type = uint16_t;

	static constexpr entity_type entity_mask = 0x3FFFF;
	static constexpr entity_type version_mask = 0x3FFF;
};

template <>
struct entt::entt_traits<EntityId> : basic_entt_traits<EntityIdTraits> {
    static constexpr std::size_t page_size = 2048;
};

template<std::derived_from<IEntityComponent> Type>
struct entt::component_traits<Type, EntityId> {
    using element_type = Type;
    using entity_type = EntityId;
    static constexpr bool in_place_delete = true;
    static constexpr std::size_t page_size = 128 * !std::is_empty_v<Type>;
};

template<typename Type>
struct entt::storage_type<Type, EntityId> {
    using type = basic_storage<Type, EntityId>;
};

class EntityRegistry : public std::enable_shared_from_this<EntityRegistry> {
public:
	std::string name;
	entt::basic_registry<EntityId> ownedRegistry;
	uint32_t id;

    template<typename T>
    T* tryGetGlobalComponent() {
        return this->ownedRegistry.ctx().find<T>();
    }
};

class EntityContext {
public:
	EntityRegistry& registry;
	entt::basic_registry<EntityId>& enttRegistry;
	EntityId entity;
};

class StrictEntityContext {
public:
    EntityId entity;
    uint32_t registryId;
};
