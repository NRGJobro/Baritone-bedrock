#pragma once

#include "../Sig/SignatureManager.h"
#include "Hook.h"

#define ADD_HOOK(name, func) HookManager::addHook(GET_SIG(name), reinterpret_cast<void*>(func))
#define ADD_HOOK2(func, addr) HookManager::addHook(addr, reinterpret_cast<void*>(func))
#define GET_HOOK(func) HookManager::getReturn<func>()

class HookManager {
public:
    // Every real hook callback owns one of these for its entire execution.
    // Teardown disables hook entry first, then waits until all guards drain
    // before freeing MinHook trampolines or module state.
    class CallbackGuard {
    public:
        CallbackGuard();
        ~CallbackGuard();

        CallbackGuard(const CallbackGuard&) = delete;
        CallbackGuard& operator=(const CallbackGuard&) = delete;

        [[nodiscard]] bool allowClientCode() const;
    };

    static void initializeHooks();

    static void setHooksEnabled(bool enabled);
    static void beginShutdown();
    static void waitForCallbacks();
    [[nodiscard]] static bool isShuttingDown();

    static Hook* getHook(void* func);

    static void addHook(const uintptr_t& sig, void* c);

    template<auto callback>
    static auto getReturn() {
        const auto hook = getHook(reinterpret_cast<void*>(callback));
        return hook == nullptr ? nullptr : reinterpret_cast<decltype(callback)>(hook->original);
    }

    static void destroy();

private:
    static inline std::unordered_map<void*, std::unique_ptr<Hook>> hooks{};
    static inline std::mutex hooksMutex{};
    static inline std::atomic_bool globallyEnabled{false};
    static inline std::atomic_bool shuttingDown{false};
    static inline std::atomic_uint32_t activeCallbacks{0};
};
