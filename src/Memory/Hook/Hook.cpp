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
    if (this->enabled)
        this->disable();

    MH_RemoveHook(this->target);
}

void Hook::enable() {
    if (this->enabled || !this->valid)
        return;

    MH_EnableHook(this->target);
    this->enabled = true;
}

void Hook::disable() {
    if (!this->enabled || !this->valid)
        return;

    MH_DisableHook(this->target);
    this->enabled = false;
}

void Hook::setEnabled(const bool enabled) {
    enabled ? this->enable() : this->disable();
}
