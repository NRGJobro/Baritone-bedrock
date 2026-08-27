#include "ModuleManager.h"

#include "Modules/BaritoneModule.h"

ModuleManager g_modMgr;

void ModuleManager::init() {
    this->addModule<BaritoneModule>();
}

void ModuleManager::shutdown() {
    for (const auto& module : std::views::values(this->modules))
        module->setEnabled(false);

    this->sortedModules.clear();
    this->modules.clear();
}

void ModuleManager::onTick() {
    for (const auto& module : std::views::values(this->modules)) {
        if (module->isEnabled())
            module->onTick();
    }
}

void ModuleManager::onRenderLevel() {
    for (const auto& module : std::views::values(this->modules)) {
        if (module->isEnabled())
            module->onRenderLevel();
    }
}

bool ModuleManager::handleChat(const std::string& message) {
    const auto baritone = this->getModule<BaritoneModule>();
    return baritone != nullptr && baritone->handleChat(message);
}

size_t ModuleManager::getModuleCount() const {
    return this->modules.size();
}

const std::set<std::shared_ptr<Module>, ModuleManager::moduleSort>& ModuleManager::getSortedModules() {
    return this->sortedModules;
}

std::shared_ptr<Module> ModuleManager::getModule(const uint32_t moduleHash) {
    if (!this->modules.contains(moduleHash))
        return nullptr;

    return this->modules.at(moduleHash);
}
