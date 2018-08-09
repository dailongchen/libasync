#pragma once

#include <thread>

#include "TaskDetails.h"
#include "TaskHandle.h"

namespace Async {

    typedef enum
    {
        RunMode_Sync,
        RunMode_Async
    } RunMode;

    /////////////////////////////////////////////////
    /// class Task
    /////////////////////////////////////////////////
    template<typename ReturnType>
    class Task
    {
    public:
        Task(std::shared_ptr<TaskDetails<ReturnType> > taskDetails)
            : m_details(taskDetails)
        {}

        ~Task()
        {}

        template<typename NextTaskFunction>
        Task<FUNCTION_RETURN_TYPE(NextTaskFunction)> Then(NextTaskFunction&& func)
        {
            auto newDetails = m_details->Then(func);
            return Task<FUNCTION_RETURN_TYPE(NextTaskFunction)>(newDetails);
        }

        template<typename NextTaskFunction, typename U = ReturnType>
        Task<FUNCTION_WITH_ARGUMENT_RETURN_TYPE(NextTaskFunction, U)> Get(NextTaskFunction&& func)
        {
            auto newDetails = m_details->Get(func);
            return Task<FUNCTION_WITH_ARGUMENT_RETURN_TYPE(NextTaskFunction, U)>(newDetails);
        }

        template<typename NotifyData>
        Task & Notified(NOTIFY_FUNCTION(NotifyData) && notifyFunction)
        {
            m_details->template Notified<NotifyData>(notifyFunction);
            return *this;
        }

        Task & OnException(EXCEPTION_HANDLE_FUNCTION exceptionHandle)
        {
            m_details->OnException(exceptionHandle);
            return *this;
        }

        Task & OnBegin(std::function<void()> beginHandle)
        {
            m_details->OnBegin(beginHandle);
            return *this;
        }

        Task & OnEnd(std::function<void()> endHandle)
        {
            m_details->OnEnd(endHandle);
            return *this;
        }

        iTaskHandle::ptr Run(RunMode mode = RunMode::RunMode_Async)
        {
            std::shared_ptr<TaskDetails<ReturnType> > taskDetails = m_details;
            auto runFunction = [taskDetails]() {
                taskDetails->BeforeRun();
                try
                {
                    taskDetails->Run();
                }
                catch (...)
                {
                }
                taskDetails->AfterRun();
            };

            auto thread = std::make_shared<std::thread>(runFunction);
            if (mode == RunMode::RunMode_Sync)
                thread->join();

            auto cancelFunc = [taskDetails]() {
                taskDetails->Cancel();
            };
            auto joinFunc = [thread]() {
                if (thread && thread->joinable())
                    thread->join();
            };
            auto detachFunc = [thread]() {
                if (thread && thread->joinable())
                    thread->detach();
            };

            auto handle = TaskHandle::New(cancelFunc, joinFunc, detachFunc);
            taskDetails->Handle = handle; // hold the handle in details, until the thread end

            return handle;
        }

    private:
        std::shared_ptr<TaskDetails<ReturnType> > m_details;
    };

    /////////////////////////////////////////////////
    /// function Spawn
    /////////////////////////////////////////////////
    template<typename TaskFunction>
    Task<FUNCTION_RETURN_TYPE(TaskFunction)> Spawn(TaskFunction&& func)
    {
        auto newDetails = CreateTaskDetails(func);
        return Task<FUNCTION_RETURN_TYPE(TaskFunction)>(newDetails);
    }
}