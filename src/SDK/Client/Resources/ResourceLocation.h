#pragma once

#include "ResourceFileSystem.h"

class ResourceLocation {
public:
    ResourceFileSystem fileSystem; // 0x0
    std::string path; // 0x8
    uint64_t pathHash; // 0x28
    uint64_t fullHash; // 0x30

    static constexpr uint64_t computeHash(std::string_view str) {
        if (str.empty())
            return 0;

        uint64_t hash = 0xCBF29CE484222325;

        for (const char c : str) {
            hash *= 0x100000001B3; // 64bit Prime Multiplier
            hash ^= static_cast<uint64_t>(c);
        }

        return hash;
    }

    ResourceLocation() = default;

    ResourceLocation(std::string_view path, ResourceFileSystem fileSystem) : path(path), fileSystem(fileSystem) {
        this->pathHash = computeHash(path);
        this->fullHash = this->pathHash ^ ((*reinterpret_cast<uint8_t*>(&this->fileSystem) ^ 0xCBF29CE484222325) * 0x100000001B3);
    }

    ResourceLocation(std::string_view filePath, bool external) {
        this->path = filePath;

        if (external)
            this->fileSystem = ResourceFileSystem::Raw;

        this->pathHash = computeHash(filePath);
        this->fullHash = this->pathHash ^ ((*reinterpret_cast<uint8_t*>(&this->fileSystem) ^ 0xCBF29CE484222325) * 0x100000001B3);
    }

    [[nodiscard]] std::string toString() const {
        return "ResourceLocation(path=" + this->path + ", fileSystem=" + std::string(magic_enum::enum_name(this->fileSystem)) +
            ", pathHash=" + std::to_string(this->pathHash) + ", fullHash=" + std::to_string(this->fullHash) + ")";
    }

    bool operator<(const ResourceLocation& other) const {
        return this->path < other.path;
    }

    bool operator==(const ResourceLocation& other) const {
        return this->fullHash == other.fullHash;
    }
};
