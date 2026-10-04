#pragma once

#include "../Sig/SignatureManager.h"
#include "Hook.h"

#define ADD_HOOK(name, func) HookManager::addHook(GET_SIG(name), func)
#define ADD_HOOK2(func, addr) HookManager::addHook(addr, func)
#define GET_HOOK(func) HookManager::getReturn<func>()

class HookManager {
public:
    static void initializeHooks();

    static void setHooksEnabled(bool enabled);

    static Hook* getHook(void* func);

    static void addHook(const uintptr_t& sig, void* c);

    template<auto callback>
    static auto getReturn() {
        const auto hook = getHook(*callback);
        return hook == nullptr ? nullptr : reinterpret_cast<decltype(callback)>(hook->original);
    }

    static void destroy();

private:
    static inline std::unordered_map<void*, std::unique_ptr<Hook>> hooks{};
};
