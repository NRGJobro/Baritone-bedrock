#include "Dimension.h"

#include "../../Utils/Utils.h"

Level* Dimension::getLevel() {
    return hat::member_at<Level*>(this, 0xC0);
}

int16_t Dimension::getMinHeight() {
    return hat::member_at<int16_t>(this, 0xC8);
}

int16_t Dimension::getMaxHeight() {
    return hat::member_at<int16_t>(this, 0xCA);
}

std::shared_ptr<BlockSource> Dimension::getBlockSource() {
    return hat::member_at<std::shared_ptr<BlockSource>>(this, 0xF0);
}

DimensionID Dimension::getDimensionID() {
    return hat::member_at<DimensionID>(this, 0x180);
}

bool Dimension::hasCeiling() {
    return hat::member_at<bool>(this, 0x187); // Offset +3: 0F B6 ? ? ? ? ? 88 44 ? ? 49 8B ? 48 8B
}

ChunkSource* Dimension::getChunkSource() { // +3 : 48 8b a8 ? ? ? ? 80 be
    return hat::member_at<ChunkSource*>(this, 0x1D8);
}

float Dimension::getTimeOfDay(const int ticks, const float a) {
    return Utils::CallVFunc<29, float, int, float>(this, ticks, a);
}

float Dimension::getTimeOfDay(float a) {
    const auto level = this->getLevel();
    const auto gameRules = level->getGameRules();

    if (gameRules->getBool(GameRules::GameRulesIndex::DoDaylightCycle, false))
        a = 0.f;

    const int ticks = level->getLevelData()->getTime();

    return this->getTimeOfDay(ticks, a);
}
