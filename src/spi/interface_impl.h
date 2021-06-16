#pragma once

#include "../spi/interface.h"

class NonCopyMovable
{
public:
    NonCopyMovable() = default;

    NonCopyMovable(const NonCopyMovable& other) = delete;
    NonCopyMovable(NonCopyMovable&& other) = delete;
    NonCopyMovable& operator=(const NonCopyMovable& other) = delete;
    NonCopyMovable& operator=(NonCopyMovable&& other) = delete;
};


class SharedProxyInterface
    : public ISharedProxyInterface
    , public NonCopyMovable  // disable copying or moving
{
public:
    SharedProxyInterface()
        : NonCopyMovable()
    {

    }
};
