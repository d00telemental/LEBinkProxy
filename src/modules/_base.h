#pragma once

class IModule
{
protected:
    char* name_ = nullptr;
    bool active_ = false;

    IModule(char* name) : name_{ name } { }

public:
    IModule(const IModule& other) = delete;
    IModule& operator=(const IModule& other) = delete;

    virtual bool Activate() = 0;
    virtual void Deactivate() = 0;

    [[nodiscard]] __forceinline char* Name() const noexcept { return name_; }
    [[nodiscard]] __forceinline bool Active() const noexcept { return active_; }
};
