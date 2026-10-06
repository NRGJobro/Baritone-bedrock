#include "Module.h"

Module::Module(std::string description) : description(std::move(description)) {}

std::string Module::getName() {
    return "Module";
}

void Module::onEnable() {}

void Module::onDisable() {}

void Module::onTick() {}

void Module::onPostTick() {}

void Module::onBeforeRenderLevel() {}

void Module::onAfterRenderLevel() {}

void Module::onRenderLevel() {}

const std::string& Module::getDescription() {
    return this->description;
}

void Module::toggle() {
    this->setEnabled(!this->enabled);
}

void Module::setEnabled(const bool enabled) {
    // Re-running onDisable during teardown/world transitions can touch game
    // objects that are already being destroyed. Only fire lifecycle callbacks
    // on a real state transition.
    if (this->enabled == enabled)
        return;

    this->enabled = enabled;

    if (enabled)
        this->onEnable();
    else
        this->onDisable();
}

bool Module::isEnabled() const {
    return this->enabled;
}
