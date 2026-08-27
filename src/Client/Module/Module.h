#pragma once

class Module {
    std::string description{};
    bool enabled = false;

protected:
    explicit Module(std::string description);

public:
    float enabledButtonRegion = 0.f;

    virtual ~Module() = default;
    virtual std::string getName();
    virtual void onEnable();
    virtual void onDisable();
    virtual void onTick();
    virtual void onRenderLevel();

    const std::string& getDescription();
    void toggle();
    void setEnabled(bool enabled);
    [[nodiscard]] bool isEnabled() const;
};
