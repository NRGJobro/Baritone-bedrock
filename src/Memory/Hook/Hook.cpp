#include "Hook.h"

#include "../../Utils/Logger.h"

Hook::Hook(const uintptr_t& target, void* callback) {
    this->target = reinterpret_cast<void*>(target);
    this->callback = callback;

    const auto status = MH_CreateHook(this->target, this->callback, &this->original);

    this->valid = status == MH_OK;

#ifndef NDEBUG
    if (!this->valid)
        logF("Failed to create hook: {} ({:#x})", magic_enum::enum_name(status), target);
#endif
}

Hook::~Hook() {
    if (!this->valid)
        return;

    if (this->enabled)
        this->disable();

    MH_RemoveHook(this->target);
}

void Hook::enable() {
    if (this->enabled || !this->valid)
        return;

    const auto status = MH_EnableHook(this->target);
    if (status == MH_OK || status == MH_ERROR_ENABLED)
        this->enabled = true;
#ifndef NDEBUG
    else
        logF("Failed to enable hook: {} ({:#x})", magic_enum::enum_name(status),
            reinterpret_cast<std::uintptr_t>(this->target));
#endif
}

void Hook::disable() {
    if (!this->enabled || !this->valid)
        return;

    const auto status = MH_DisableHook(this->target);
    if (status == MH_OK || status == MH_ERROR_DISABLED)
        this->enabled = false;
#ifndef NDEBUG
    else
        logF("Failed to disable hook: {} ({:#x})", magic_enum::enum_name(status),
            reinterpret_cast<std::uintptr_t>(this->target));
#endif
}

void Hook::setEnabled(const bool enabled) {
    enabled ? this->enable() : this->disable();
}
