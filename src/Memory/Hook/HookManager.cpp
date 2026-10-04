#include "HookManager.h"

#include "Hooks/ClientHooks.h"
#include "Hooks/RenderHooks.h"

void HookManager::setHooksEnabled(const bool enabled) {
    if (enabled)
        MH_EnableHook(MH_ALL_HOOKS);
    else
        MH_DisableHook(MH_ALL_HOOKS);
}

Hook* HookManager::getHook(void* func) {
    return hooks[func].get();
}

void HookManager::addHook(const uintptr_t& sig, void* c) {
    if (sig == 0 || c == nullptr) {
        logF("Refusing to create a hook with a null target/callback");
        return;
    }
    auto hook = std::make_unique<Hook>(sig, c);
    hooks.emplace(c, std::move(hook));
}

void HookManager::destroy() {
    hooks.clear();
}

void HookManager::initializeHooks() {
    std::vector<std::thread> threads;

    threads.emplace_back(&ClientHooks::init);
    threads.emplace_back(&RenderHooks::init);

    for (auto& t : threads) {
        t.join();
    }
}
