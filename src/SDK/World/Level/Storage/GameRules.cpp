#include "GameRules.h"

const std::vector<GameRule>& GameRules::getGameRules() {
    return hat::member_at<std::vector<GameRule>>(this, 0x18);
}

bool GameRules::getBool(const GameRulesIndex gameRule, const bool defaultValue) {
    const int i = magic_enum::enum_integer(gameRule);

    const auto& gameRules = this->getGameRules();

    if (i >= gameRules.size())
        return defaultValue;

    const auto& rule = gameRules[i];

    if (rule.type != GameRule::Type::Bool)
        return defaultValue;

    return rule.value.boolVal;
}
