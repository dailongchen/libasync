#pragma once

#include "ThreadLocal.h"

#define NOTIFY_FUNCTION(NotifyData) std::function<void(const NotifyData&)>

namespace Async {

    /////////////////////////////////////////////////
    /// function GetNotifier
    /////////////////////////////////////////////////
    template<typename NotifyData>
    class Notifier;

    template<typename NotifyData>
    Notifier<NotifyData> ** GetNotifier(bool createIfNoExist = true)
    {
        THREAD_LOCAL static Notifier<NotifyData> *notifier = nullptr;

        if (createIfNoExist && notifier == nullptr)
            notifier = new Notifier<NotifyData>();

        return &notifier;
    }

    /////////////////////////////////////////////////
    /// class Notifier
    /////////////////////////////////////////////////
    template<typename NotifyData>
    class Notifier
    {
    public:
        Notifier()
            : m_notifyFunction(nullptr)
        {}

        void SetFunction(NOTIFY_FUNCTION(NotifyData) function)
        {
            m_notifyFunction = function;
        }

        NOTIFY_FUNCTION(NotifyData) & GetFunction()
        {
            return m_notifyFunction;
        }

    private:
        NOTIFY_FUNCTION(NotifyData) m_notifyFunction;
    };

    template<typename NotifyData>
    void SetNotifyFunction(NOTIFY_FUNCTION(NotifyData) notifyFunction)
    {
        if (notifyFunction == nullptr) // you have to release notifier in this way
        {
            auto notifierPtr = GetNotifier<NotifyData>(false); // return nullptr if not exist
            if (*notifierPtr)
            {
                delete *notifierPtr;
                *notifierPtr = nullptr;
            }
        }
        else
        {
            auto notifierPtr = GetNotifier<NotifyData>();
            (*notifierPtr)->SetFunction(notifyFunction);
        }
    }

    template<typename NotifyData>
    NOTIFY_FUNCTION(NotifyData) GetNotifyFunction()
    {
        auto notifierPtr = GetNotifier<NotifyData>(false);

        if (*notifierPtr)
            return (*notifierPtr)->GetFunction();
        return nullptr;
    }
}