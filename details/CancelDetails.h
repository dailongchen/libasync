#pragma once

#include <future>

#include "ThreadLocal.h"

namespace Async {

    /////////////////////////////////////////////////
    /// class CancelTrigger
    /////////////////////////////////////////////////
    class CancelTrigger
    {
    public:
        CancelTrigger() : m_cancel(false)
        {}

        // cancel immediately, shouldn't be ignored
        void Set(bool value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_cancel = value;
        }

        bool Get() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_cancel;
        }

        static CancelTrigger ** GetCancelTrigger()
        {
            THREAD_LOCAL static CancelTrigger *cancelTrigger = nullptr;

            return &cancelTrigger;
        }

    private:
        bool m_cancel; // cancel immediately, shouldn't be ignored
        mutable std::mutex m_mutex;
    };
}
