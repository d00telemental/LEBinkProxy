#pragma once
#include "dllincludes.h"
#include "io.h"

namespace Sync
{
    enum class EventWaitValue
    {
        Abandoned = 0,
        Signaled = 1,
        Timeout = 2,
        Failed = 3,
        ArbitrarilyFailed = 4
    };

    struct Event
    {
    private:
        HANDLE handle_;
        const wchar_t* name_;
        bool inError_;

    public:
        Event(const wchar_t* name)
            : name_{ name }
            , inError_{ true }
        {
            handle_ = CreateEvent(nullptr, true, false, name_);
            if (!handle_)
            {
                GLogger.writeFormatLine(L"Event(): failed to create an event (%s), error code = %d", name_ ? name_ : L"(none)", GetLastError());
                return;
            }
            inError_ = false;
        }

        ~Event()
        {
            if (handle_)
            {
                CloseHandle(handle_);
            }
        }

        bool Set()
        {
            if (inError_)
            {
                GLogger.writeFormatLine(L"Event.Set: aborting becase the object is in error state");
                return false;
            }
            return SetEvent(handle_);
        }

        bool Reset()
        {
            if (inError_)
            {
                GLogger.writeFormatLine(L"Event.Reset: aborting becase the object is in error state");
                return false;
            }
            return ResetEvent(handle_);
        }

        EventWaitValue WaitForIt(int timeout)
        {
            if (inError_)
            {
                GLogger.writeFormatLine(L"Event.WaitForIt: aborting becase the object is in error state");
                return EventWaitValue::ArbitrarilyFailed;
            }

            auto rc = WaitForSingleObject(handle_, timeout);
            switch (rc)
            {
            case WAIT_ABANDONED:
                return EventWaitValue::Abandoned;
            case WAIT_OBJECT_0:
                return EventWaitValue::Signaled;
            case WAIT_TIMEOUT:
                return EventWaitValue::Timeout;
            case WAIT_FAILED:
                return EventWaitValue::Failed;
            default:
                GLogger.writeFormatLine(L"Event.WaitForIt: unknown return code (%d)", rc);
                return EventWaitValue::Failed;
            }
        }

        bool InError() const noexcept { return inError_; }
    };
}
