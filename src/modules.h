#pragma once
#include "dllincludes.h"


class IModule
{
protected:
    char* name_ = nullptr;
    bool active_ = false;

    IModule(char* name) : name_{ name } { }

public:
    IModule(const IModule& other) = delete;
    IModule& operator=(const IModule& other) = delete;

    virtual bool Register(void* pData) = 0;
    virtual bool Activate() = 0;
    virtual void Deactivate() = 0;

    [[nodiscard]] __forceinline char* Name() const noexcept { return name_; }
    [[nodiscard]] __forceinline bool Active() const noexcept { return active_; }
};

template<int CapacityT = 64>
class ModuleList
{
private:
    typedef IModule* IModulePtrT;
    typedef IModule& IModuleRefT;

    IModulePtrT data_[CapacityT];
    int count_ = 0;

    IModule* findModule_(char* name) const noexcept
    {
        for (int m = 0; m < count_; m++)
        {
            if (data_[m] && 0 == strcmp(data_[m]->Name(), name))
            {
                return data_[m];
            }
        }
        return nullptr;
    }

    bool hasModule_(char* name) const noexcept
    {
        return findModule_(name) != nullptr;
    }

public:
    ModuleList()
        : count_{ 0 }
    {

    }

    ~ModuleList()
    {
        for (int idx = 0; idx < count_; idx++)
        {
            delete data_[idx];
        }
    }

    ModuleList(const ModuleList& other) = delete;
    ModuleList& operator=(const ModuleList& other) = delete;

    bool Register(IModulePtrT item)
    {
        if (count_ > capacity)
        {
            return false;
        }

        if (hasModule_(item->Name()))
        {
            return false;
        }

        data[count_++] = item;
        return data[count_ - 1]->Register(nullptr);
    }

    bool Activate(char* name)
    {
        auto modulePtr = findModule_(name);
        if (!modulePtr || modulePtr->Active())
        {
            return false;
        }
        return modulePtr->Activate();
    }

    [[nodiscard]] __forceinline int Capacity() const noexcept { return CapacityT; }
    [[nodiscard]] __forceinline int Count() const noexcept { return count_; }
};
