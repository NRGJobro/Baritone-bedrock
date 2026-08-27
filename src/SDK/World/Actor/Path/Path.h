#pragma once

#include "NodeType.h"
#include "PathCompletionType.h"

struct Path {
    struct Node {
        glm::ivec3 pos;
        NodeType type;
    };

    std::vector<Node> nodes;
    uint64_t index;
    PathCompletionType completionType;
};
