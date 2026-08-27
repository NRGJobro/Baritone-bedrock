#pragma once

#include "../Components/IEntityComponent.h"

class EntityId;

struct EntityIdTraits {
	using value_type = EntityId;

	using entity_type = uint32_t;
	using version_type = uint16_t;

	static constexpr entity_type entity_mask = 0x3FFFF;
	static constexpr entity_type version_mask = 0x3FFF;
};

template<typename Type>
struct entt::storage_type<Type, EntityId> {
    using type = basic_storage<Type, EntityId>;
};

template <>
class entt::entt_traits<EntityId> : public entt::basic_entt_traits<EntityIdTraits> {
public:
	static constexpr entity_type page_size = 2048;
};

class EntityId : public entt::entt_traits<EntityId> {
public:
	entity_type rawId{};

	EntityId() = default;

	template <std::integral T>
		requires(!std::is_same_v<std::remove_cvref_t<T>, bool>)
	constexpr EntityId(T rawId) : rawId(static_cast<entity_type>(rawId)) {}  // NOLINT

	constexpr bool isNull() const { return *this == entt::null; }

	template <std::integral T>
		requires(!std::is_same_v<std::remove_cvref_t<T>, bool>)
	constexpr operator T() const {
		return static_cast<T>(rawId);
	}

    [[nodiscard]] constexpr bool operator==(const EntityId& other) const {
	    return this->rawId == other.rawId;
	}

    [[nodiscard]] constexpr operator entity_type() const {
        return this->rawId;
    }
};

template<std::derived_from<IEntityComponent> Type>
struct entt::component_traits<Type> {
    using type = Type;
    static constexpr bool in_place_delete = true;
    static constexpr std::size_t page_size = 128 * !std::is_empty_v<Type>;
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
