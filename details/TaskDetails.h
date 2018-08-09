#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "Cancel.h"
#include "ExceptionDetails.h"
#include "Notify.h"
#include "TaskHandle.h"

#define FUNCTION_WITH_ARGUMENT_RETURN_TYPE(Function, Argument) typename std::result_of<Function&&(Argument)>::type
#define FUNCTION_RETURN_TYPE(Function) typename std::result_of<Function&&()>::type

namespace Async {

    typedef struct
    {
        bool Bypass;
    } TaskBypassFlag;

    /////////////////////////////////////////////////
    /// class TaskDetails
    /////////////////////////////////////////////////
    template<typename ReturnType>
    class TaskDetails : public std::enable_shared_from_this<TaskDetails<ReturnType> >
    {
    private:
        typedef std::function<void()> VoidFunction;

    public:
        TaskDetails(std::function<ReturnType()>&& func, std::shared_ptr<TaskBypassFlag> bypassFlag = nullptr)
            : m_function(func), m_bypassFlag(bypassFlag),
              m_onEndFunction(nullptr), m_onBeginFunction(nullptr),
              m_exceptionHandle(nullptr)
        {
        }

        ~TaskDetails()
        {
            //std::cout << typeid(ReturnType).name() << std::endl;
        }

        void BeforeRun()
        {
            if (m_bypassFlag)
                m_bypassFlag->Bypass = false;

            m_cancelTrigger = new CancelTrigger();
            *(CancelTrigger::GetCancelTrigger()) = m_cancelTrigger;

            std::for_each(m_notifierInitializer.begin(), m_notifierInitializer.end(),
                [](const VoidFunction& initFunc) {
                initFunc();
            });

            if (m_onBeginFunction)
                m_onBeginFunction();
        }

        ReturnType Run()
        {
            try
            {
                if (m_function)
                    return m_function();
                return ReturnType();
            }
            catch (...)
            {
                if (m_exceptionHandle)
                    m_exceptionHandle(std::current_exception());
                throw;
            }
        }

        void AfterRun()
        {
            if (m_onEndFunction)
                m_onEndFunction();

            std::for_each(m_notifierReleaser.begin(), m_notifierReleaser.end(),
                [](const VoidFunction& releaseFunc) {
                releaseFunc();
            });

            *(CancelTrigger::GetCancelTrigger()) = nullptr;
            delete m_cancelTrigger;
            m_cancelTrigger = nullptr;

            Handle = nullptr; // release the Handle shared_ptr here
        }

        void Cancel()
        {
            if (m_cancelTrigger)
                m_cancelTrigger->Set(true);
        }

        bool IsBypass() const
        {
            if (m_bypassFlag)
                return m_bypassFlag->Bypass;
            return false;
        }

        template<typename NextTaskFunction>
        std::shared_ptr<TaskDetails<FUNCTION_RETURN_TYPE(NextTaskFunction)> > Then(NextTaskFunction&& func)
        {
            /*
            std::string ReturnTypeName(typeid(FUNCTION_RETURN_TYPE(NextTaskFunction)).name());
            */
            auto parent = this->shared_from_this();
            auto functionWrapper = [parent, func]() {
                parent->Run();

                if (parent->m_bypassFlag && parent->m_bypassFlag->Bypass)
                    return FUNCTION_RETURN_TYPE(NextTaskFunction)();

                return func();
            };

            return std::make_shared<TaskDetails<FUNCTION_RETURN_TYPE(NextTaskFunction)> >(
                std::forward<std::function<FUNCTION_RETURN_TYPE(NextTaskFunction)()> >(functionWrapper),
                m_bypassFlag
            );
        }

        template<typename NextTaskFunction, typename U = ReturnType>
        std::shared_ptr<TaskDetails<FUNCTION_WITH_ARGUMENT_RETURN_TYPE(NextTaskFunction, U)> > Get(NextTaskFunction&& func)
        {
            /*
            std::string NextReturnTypeName(typeid(FUNCTION_WITH_ARGUMENT_RETURN_TYPE(NextTaskFunction, U)).name());
            std::string ParentReturnTypeName(typeid(U).name());
            */
            auto parent = this->shared_from_this();
            auto functionWrapper = [parent, func]() {
                auto parentReturn = parent->Run();

                if (parent->m_bypassFlag && parent->m_bypassFlag->Bypass)
                    return FUNCTION_WITH_ARGUMENT_RETURN_TYPE(NextTaskFunction, U)();

                return func(parentReturn);
            };

            return std::make_shared<TaskDetails<FUNCTION_WITH_ARGUMENT_RETURN_TYPE(NextTaskFunction, U)> >(
                std::forward<std::function<FUNCTION_WITH_ARGUMENT_RETURN_TYPE(NextTaskFunction, U)()> >(functionWrapper),
                m_bypassFlag
            );
        }

        template<typename NotifyData>
        void Notified(NOTIFY_FUNCTION(NotifyData) notifyFunction)
        {
            m_notifierInitializer.push_back([notifyFunction] {
                SetNotifyFunction<NotifyData>(notifyFunction);
            });

            m_notifierReleaser.push_back([] {
                SetNotifyFunction<NotifyData>(nullptr);
            });
        }

        void OnException(EXCEPTION_HANDLE_FUNCTION exceptionHandle)
        {
            m_exceptionHandle = exceptionHandle;
        }

        void OnBegin(std::function<void()> onBeginFunction)
        {
            m_onBeginFunction = onBeginFunction;
        }

        void OnEnd(std::function<void()> onEndFunction)
        {
            m_onEndFunction = onEndFunction;
        }

    public:
        iTaskHandle::ptr Handle;

    private:
        std::function<ReturnType()> m_function;
        CancelTrigger *m_cancelTrigger;
        std::shared_ptr<TaskBypassFlag> m_bypassFlag;

        std::vector<std::function<void()> > m_notifierInitializer;
        std::vector<std::function<void()> > m_notifierReleaser;

        std::function<void()> m_onEndFunction;
        std::function<void()> m_onBeginFunction;

        EXCEPTION_HANDLE_FUNCTION m_exceptionHandle;
    };

    /////////////////////////////////////////////////
    /// function CreateTaskDetails
    /////////////////////////////////////////////////
    template<typename TaskFunction>
    std::shared_ptr<TaskDetails<FUNCTION_RETURN_TYPE(TaskFunction)> > CreateTaskDetails(TaskFunction&& func)
    {
        return std::make_shared<TaskDetails<FUNCTION_RETURN_TYPE(TaskFunction)> >(
            std::forward<TaskFunction>(func)
        );
    }
}
