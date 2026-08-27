#pragma once

#include "Module.h"

class ModuleManager {
    struct moduleSort {
        bool operator()(const std::shared_ptr<Module>& first, const std::shared_ptr<Module>& second) const {
            return first->getName() < second->getName();
        }
    };

    std::map<uint32_t, std::shared_ptr<Module>> modules;
    std::set<std::shared_ptr<Module>, moduleSort> sortedModules;

    template<typename T>
    void addModule() {
        auto mod = std::make_shared<T>();
        this->modules.insert({entt::type_hash<T>::value(), mod});
        this->sortedModules.emplace(mod);
    }

public:
    void init();
    void shutdown();
    void onTick();
    void onRenderLevel();
    bool handleChat(const std::string& message);

    [[nodiscard]] size_t getModuleCount() const;

    const std::set<std::shared_ptr<Module>, moduleSort>& getSortedModules();

    std::shared_ptr<Module> getModule(uint32_t moduleHash);

    template <typename T>
    T* getModule() {
        const auto id = entt::type_hash<T>::value();

        const auto mod = getModule(id);

        if (mod == nullptr)
            return nullptr;

        return reinterpret_cast<T*>(mod.get());
    }
};

extern ModuleManager g_modMgr;
