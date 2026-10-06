#include "HookManager.h"

#include "Hooks/ClientHooks.h"
#include "Hooks/RenderHooks.h"

HookManager::CallbackGuard::CallbackGuard() {
    HookManager::activeCallbacks.fetch_add(1, std::memory_order_acq_rel);
}

HookManager::CallbackGuard::~CallbackGuard() {
    HookManager::activeCallbacks.fetch_sub(1, std::memory_order_acq_rel);
}

bool HookManager::CallbackGuard::allowClientCode() const {
    return !HookManager::shuttingDown.load(std::memory_order_acquire);
}

bool HookManager::isShuttingDown() {
    return shuttingDown.load(std::memory_order_acquire);
}

void HookManager::beginShutdown() {
    shuttingDown.store(true, std::memory_order_release);
    globallyEnabled.store(false, std::memory_order_release);
    MH_DisableHook(MH_ALL_HOOKS);
}

void HookManager::waitForCallbacks() {
    // A callback can already be inside a trampoline when hooks are disabled.
    // Keep every trampoline/module object alive until those callbacks return.
    while (activeCallbacks.load(std::memory_order_acquire) != 0)
        Sleep(1);
}

void HookManager::setHooksEnabled(const bool enabled) {
    if (enabled && shuttingDown.load(std::memory_order_acquire))
        return;

    globallyEnabled.store(enabled, std::memory_order_release);
    if (enabled)
        MH_EnableHook(MH_ALL_HOOKS);
    else
        MH_DisableHook(MH_ALL_HOOKS);
}

Hook* HookManager::getHook(void* func) {
    const std::scoped_lock lock(hooksMutex);
    const auto found = hooks.find(func);
    return found == hooks.end() ? nullptr : found->second.get();
}

void HookManager::addHook(const uintptr_t& sig, void* c) {
    if (shuttingDown.load(std::memory_order_acquire))
        return;

    if (sig == 0 || c == nullptr) {
        logF("Refusing to create a hook with a null target/callback");
        return;
    }
    auto hook = std::make_unique<Hook>(sig, c);
    const std::scoped_lock lock(hooksMutex);
    const auto [entry, inserted] = hooks.emplace(c, std::move(hook));
    if (inserted && globallyEnabled.load(std::memory_order_acquire))
        entry->second->enable();
}

void HookManager::destroy() {
    waitForCallbacks();
    const std::scoped_lock lock(hooksMutex);
    hooks.clear();
}

void HookManager::initializeHooks() {
    shuttingDown.store(false, std::memory_order_release);
    activeCallbacks.store(0, std::memory_order_release);

    std::vector<std::thread> threads;

    threads.emplace_back(&ClientHooks::init);
    threads.emplace_back(&RenderHooks::init);

    for (auto& t : threads) {
        t.join();
    }
}
