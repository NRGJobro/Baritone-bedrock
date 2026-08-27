#pragma once

#include "../Bedrock/StaticOptimizedString.h"

class SemVersion {
public:
    enum class MatchType : int {
        Full,
        Partial,
        None
    };

    enum class ParseOption : int {
        AllowWildcards,
        NoWildcards
    };

    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    bool validVersion;
    bool anyVersion;
    Bedrock::StaticOptimizedString preRelease;
    Bedrock::StaticOptimizedString buildMeta;

    SemVersion& operator=(const SemVersion& version) {
        this->major = version.major;
        this->minor = version.minor;
        this->patch = version.patch;
        this->validVersion = version.validVersion;
        this->anyVersion = version.anyVersion;
        this->preRelease = version.preRelease;
        this->buildMeta = version.buildMeta;

        return *this;
    }
};
