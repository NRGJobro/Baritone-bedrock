#pragma once

class Hook {
public:
    Hook() = default;

    Hook(const uintptr_t& target, void* callback);
    ~Hook();

    void enable();
    void disable();
    void setEnabled(bool enabled);

    void* target = nullptr;
    void* callback = nullptr;
    void* original = nullptr;
    bool enabled = false;

private:
    bool valid = false;
};
