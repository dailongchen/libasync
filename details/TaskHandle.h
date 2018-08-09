#pragma once

#include <functional>
#include <memory>

namespace Async {

    /////////////////////////////////////////////////
    /// interface iTaskHandle
    /////////////////////////////////////////////////
    class iTaskHandle
    {
    public:
        typedef std::shared_ptr<iTaskHandle> ptr;
        typedef std::weak_ptr<iTaskHandle> weakPtr;

    public:
        virtual ~iTaskHandle()
        {}

        virtual void Cancel() = 0;
        virtual void Join() = 0;
    };

    /////////////////////////////////////////////////
    /// class TaskHandle
    /////////////////////////////////////////////////
    class TaskHandle : public iTaskHandle
    {
    public:
        virtual ~TaskHandle()
        {
            if (m_detachFunc)
                m_detachFunc();
        }

        static iTaskHandle::ptr New(std::function<void()> cancelFunc,
            std::function<void()> joinFunc,
            std::function<void()> detachFunc)
        {
            auto newHandle = new TaskHandle();
            newHandle->m_cancelFunc = cancelFunc;
            newHandle->m_joinFunc = joinFunc;
            newHandle->m_detachFunc = detachFunc;

            return iTaskHandle::ptr((iTaskHandle *)newHandle);
        }

        virtual void Cancel()
        {
            if (m_cancelFunc)
                m_cancelFunc();
        }

        virtual void Join()
        {
            if (m_joinFunc)
                m_joinFunc();
        }

    private:
        TaskHandle()
            : m_cancelFunc(nullptr), m_joinFunc(nullptr)
        {}

    private:
        std::function<void()> m_cancelFunc;
        std::function<void()> m_joinFunc;
        std::function<void()> m_detachFunc;
    };
}
