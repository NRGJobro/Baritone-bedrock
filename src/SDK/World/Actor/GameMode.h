#pragma once

#include "../Block/Block.h"
#include "../Level/HitResult/FacingID.h"
#include <glm/glm.hpp>

class Actor;
class ItemStack;

// Same-version Bedrock GameMode interface.  Placement is routed through the
// game's normal interaction code so reach, permissions and inventory are
// validated by the engine instead of forging packets.
class GameMode {
    virtual void destructor();
public:
    virtual bool startDestroyBlock(const glm::ivec3&, FacingID, bool&);
    virtual bool destroyBlock(glm::ivec3*, FacingID);
    virtual bool continueDestroyBlock(const glm::ivec3&, FacingID, const glm::vec3&, bool&);
    virtual void stopDestroyBlock(const glm::ivec3&);
    virtual void startBuildBlock(const glm::ivec3&, FacingID);
    virtual bool buildBlock(glm::ivec3*, FacingID, bool isSimTick = false);
    virtual void continueBuildBlock(const glm::ivec3&, FacingID);
};
