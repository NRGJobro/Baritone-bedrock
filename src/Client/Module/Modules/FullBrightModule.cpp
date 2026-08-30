#include "FullBrightModule.h"

FullBrightModule::FullBrightModule()
    : Module("Forces maximum world brightness without applying a potion effect") {}

std::string FullBrightModule::getName() { return "FullBright"; }

float FullBrightModule::getIntensity() const { return intensity; }
