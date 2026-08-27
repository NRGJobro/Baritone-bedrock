#pragma once

template <typename T>
class SubChunkStorage {
public:
    virtual ~SubChunkStorage() = default; // No need for full vtable yet
};
