#include "Module.h"

Module::Module(std::string description) : description(std::move(description)) {}

std::string Module::getName() {
    return "Module";
}

void Module::onEnable() { }

void Module::onDisable() { }

void Module::onTick() { }

void Module::onRenderLevel() { }

const std::string& Module::getDescription() {
    return this->description;
}

void Module::toggle() {
    this->setEnabled(!this->enabled);
}

void Module::setEnabled(const bool enabled) {
    this->enabled = enabled;

    if (enabled)
        this->onEnable();
    else
        this->onDisable();
}

bool Module::isEnabled() const {
    return this->enabled;
}
