#pragma once

#include "../../../Resources/BaseGameVersion.h"

class GameRule {
public:
    enum class Type : uint8_t {
        Invalid,
        Bool,
        Int,
        Float
    };

    union Value {
        bool boolVal;
        int intVal;
        float floatVal;
    };

    class ValidationError {
    public:
        bool success;
        std::string errorDescription;
        std::vector<std::string> errorParameters;

    private:
        ~ValidationError();
    };

    bool shouldSave;
    Type type;
    Value value;
    std::string name;
    bool allowUseInCommand;
    bool allowUseInScripting;
    bool isDefaultSet;
    bool requiresCheats;
    bool canBeModifiedByPlayer;
    std::function<void(GameRule&, const BaseGameVersion&)> tagNotFoundCallback;
    std::function<bool(const Value&, ValidationError*)> validateValueCallback;
};
