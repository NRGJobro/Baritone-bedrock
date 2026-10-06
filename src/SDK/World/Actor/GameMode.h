#pragma once

#include <glm/glm.hpp>

class Actor;
class ItemStack;
enum class FacingID : std::int8_t;
enum class InputMode : std::uint32_t;

// Same-version Bedrock GameMode interface.  Placement is routed through the
// game's normal interaction code so reach, permissions and inventory are
// validated by the engine instead of forging packets.
class GameMode {
    virtual void destructor();
public:
    virtual std::int64_t startDestroyBlock(const glm::ivec3&, std::uint8_t, bool&);
    virtual std::int64_t destroyBlock(const glm::ivec3&, std::uint8_t);
    virtual std::int64_t continueDestroyBlock(const glm::ivec3&, std::uint8_t, const glm::vec3&, bool&);
    virtual std::int64_t stopDestroyBlock(const glm::ivec3&);
    virtual std::int64_t startBuildBlock(const glm::ivec3&, std::uint8_t, bool auth);
    virtual std::int64_t buildBlock(const glm::ivec3&, FacingID, bool auth = false);
    virtual std::int64_t continueBuildBlock(const glm::ivec3&, std::uint8_t);
    virtual std::int64_t stopBuildBlock();
    virtual std::int64_t tick();
    virtual std::int64_t getPickRange(const InputMode&, bool);
    virtual bool useItem(ItemStack&, std::uint8_t mode = 0);
};
