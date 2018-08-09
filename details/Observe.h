#pragma once

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <deque>
#include <thread>

#include "TaskDetails.h"
#include "TaskHandle.h"

namespace Async {

    typedef struct _ObservableQueuePopResult_
    {
        _ObservableQueuePopResult_(bool success = false, bool closed = false)
            : m_success(success), m_closed(closed)
        {}

        bool IsSuccess() const
        {
            return m_success;
        }

        bool IsClosed() const
        {
            return m_closed;
        }

    private:
        bool m_success; // indicate this pop returns one or some objects
        bool m_closed; // indicate the queue has been closed

    } ObservableQueuePopResult;

    /////////////////////////////////////////////////
    /// class ObservableQueue
    /////////////////////////////////////////////////
    template<typename ObjectType>
    class ObservableQueue
    {
    public:
        ~ObservableQueue()
        {
            if (m_onCompleted)
                m_onCompleted();
        }

        static std::shared_ptr<ObservableQueue<ObjectType> >
        New(std::function<void()> onCompleted = nullptr,
            size_t limitation = SIZE_MAX)
        {
            return std::shared_ptr<ObservableQueue<ObjectType> >
                (new ObservableQueue<ObjectType>(onCompleted, limitation));
        }

        void Close()
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            m_closed = true;
        }

        void PushOne(const ObjectType& object)
        {
            while (true)
            {
                std::lock_guard<std::mutex> lock(m_mutex);

                if (m_closed) // if closed, do nothing
                    return;

                if (Async::Cancel::IsCancelled())
                    return;

                if (m_queue.size() >= m_limitation)
                {
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                    continue;
                }

                m_queue.push_back(object);
                m_cv.notify_all();

                break;
            }
        }

        template<typename ObjectTypeContainer>
        void PushSome(const ObjectTypeContainer& objects)
        {
            while (true)
            {
                std::lock_guard<std::mutex> lock(m_mutex);

                if (m_closed) // if closed, do nothing
                    return;

                if (Async::Cancel::IsCancelled())
                    return;

                if (m_queue.size() >= m_limitation)
                {
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                    continue;
                }

                m_queue.insert(m_queue.end(), objects.begin(), objects.end());
                m_cv.notify_all();

                break;
            }
        }

        ObservableQueuePopResult PopOne(ObjectType& obj)
        {
            std::unique_lock<std::mutex> lock(m_mutex);

            if (m_queue.empty())
                m_cv.wait_for(lock, std::chrono::milliseconds(300));

            if (m_queue.empty())
                return ObservableQueuePopResult(false, m_closed);

            obj = m_queue.front();
            m_queue.pop_front();

            return ObservableQueuePopResult(true, false);
        }

        ObservableQueuePopResult PopSome(std::vector<ObjectType>& vector)
        {
            std::unique_lock<std::mutex> lock(m_mutex);

            if (m_queue.empty())
                m_cv.wait_for(lock, std::chrono::milliseconds(300));

            if (m_queue.empty())
                return ObservableQueuePopResult(false, m_closed);

            std::deque<ObjectType> emptyQueue;
            m_queue.swap(emptyQueue);

            vector.insert(vector.end(), emptyQueue.begin(), emptyQueue.end());

            return ObservableQueuePopResult(true, false);
        }

    private:
        // force to always init using New()
        ObservableQueue(std::function<void()> onCompleted,
                        size_t limitation)
        : m_limitation(limitation),
          m_closed(false),
          m_onCompleted(onCompleted)
        {}

    private:
        const size_t m_limitation;
        bool m_closed;
        std::function<void()> m_onCompleted;

        std::deque<ObjectType> m_queue;
        std::mutex m_mutex;
        std::condition_variable m_cv;
    };

    /////////////////////////////////////////////////
    /// class ObserveTask
    /////////////////////////////////////////////////
    template<typename ReturnType>
    class ObserveTask
    {
    public:
        ObserveTask(std::shared_ptr<TaskDetails<ReturnType> > taskDetails)
            : m_details(taskDetails)
        {
        }

        ObserveTask(const ObserveTask& other)
            : m_details(other.m_details)
        {
        }

        ~ObserveTask()
        {
        }

        template<typename NextTaskFunction>
        ObserveTask<FUNCTION_RETURN_TYPE(NextTaskFunction)> Then(NextTaskFunction&& func)
        {
            auto newDetails = m_details->Then(func);
            return ObserveTask<FUNCTION_RETURN_TYPE(NextTaskFunction)>(newDetails);
        }

        template<typename NextTaskFunction, typename U = ReturnType>
        ObserveTask<FUNCTION_WITH_ARGUMENT_RETURN_TYPE(NextTaskFunction, U)> Get(NextTaskFunction&& func)
        {
            auto newDetails = m_details->Get(func);
            return ObserveTask<FUNCTION_WITH_ARGUMENT_RETURN_TYPE(NextTaskFunction, U)>(newDetails);
        }

        template<typename NotifyData>
        ObserveTask & Notified(NOTIFY_FUNCTION(NotifyData) && notifyFunction)
        {
            m_details->template Notified<NotifyData>(notifyFunction);
            return *this;
        }

        ObserveTask & OnException(EXCEPTION_HANDLE_FUNCTION exceptionHandle)
        {
            m_details->OnException(exceptionHandle);
            return *this;
        }

        ObserveTask & OnBegin(std::function<void()> beginHandle)
        {
            m_details->OnBegin(beginHandle);
            return *this;
        }

        ObserveTask & OnEnd(std::function<void()> endHandle)
        {
            m_details->OnEnd(endHandle);
            return *this;
        }

        iTaskHandle::ptr Run()
        {
            std::shared_ptr<TaskDetails<ReturnType> > taskDetails = m_details;
            auto runFunction = [taskDetails](std::shared_ptr<std::promise<void> > promise) {
                taskDetails->BeforeRun();

                bool setPromise = false;
                // 1. if IsCancelled, break the loop immediately, no matter if the queue is empty or not.
                // 2. if Join() is called without Cancel(), then I will let the loop
                // consume all rest objects in queue before exit. The way is to check Bypass flag,
                // which actually is a "reference" of IsTryingCancel().
                while (!Cancel::IsCancelled() && !taskDetails->IsBypass())
                {
                    if (!setPromise)
                    {
                        setPromise = true;
                        promise->set_value();
                    }

                    try
                    {
                        taskDetails->Run();
                    }
                    catch (...)
                    {
                    }
                }
                taskDetails->AfterRun();
            };

            auto promiseRunning = std::make_shared<std::promise<void> >();
            auto future = promiseRunning->get_future();

            auto thread = std::make_shared<std::thread>(runFunction, promiseRunning);

            future.wait(); // to make sure the taskDetails has already been running

            auto cancelFunc = [taskDetails]() {
                taskDetails->Cancel();
            };
            auto joinFunc = [thread, taskDetails]() {
                if (thread && thread->joinable())
                    thread->join();
            };
            auto detachFunc = [thread] {
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
    /// class Observable
    /////////////////////////////////////////////////
    template<typename ObjectType>
    class Observable
    {
    public:
        Observable(std::shared_ptr<ObservableQueue<ObjectType> > observableQueue)
            : m_observableQueue(observableQueue)
        {}

        /**
        ReceiveOne callback to handle Object one by one.

        @param TaskFunction.
        @return ObserveTask.
        */
        template<typename TaskFunction>
        ObserveTask<FUNCTION_WITH_ARGUMENT_RETURN_TYPE(TaskFunction, ObjectType)> ReceiveOne(TaskFunction&& taskFunction)
        {
            auto bypassFlag = std::shared_ptr<TaskBypassFlag>(new TaskBypassFlag({ false }));
            auto observableQueue = m_observableQueue;

            auto func = [observableQueue, taskFunction, bypassFlag]() {
                ObjectType obj {};
                while (true)
                {
                    auto ret = observableQueue->PopOne(obj);
                    if (ret.IsSuccess())
                        break;

                    // if the queue is empty, and closed,
                    // set the Bypass flag, and the thread function will run to exit
                    if (ret.IsClosed())
                    {
                        bypassFlag->Bypass = true;
                        return FUNCTION_WITH_ARGUMENT_RETURN_TYPE(TaskFunction, ObjectType)();
                    }
                }
                return taskFunction(obj);
            };
            auto taskDetails = std::make_shared<TaskDetails<FUNCTION_WITH_ARGUMENT_RETURN_TYPE(TaskFunction, ObjectType)> >(
                std::forward<std::function<FUNCTION_WITH_ARGUMENT_RETURN_TYPE(TaskFunction, ObjectType)()> >(func),
                bypassFlag
            );
            return ObserveTask<FUNCTION_WITH_ARGUMENT_RETURN_TYPE(TaskFunction, ObjectType)>(taskDetails);
        }

        /**
        ReceiveSome callback to handle a bunch of Objects.

        @param TaskFunction.
        @return ObserveTask.
        */
        template<typename TaskFunction>
        ObserveTask<FUNCTION_WITH_ARGUMENT_RETURN_TYPE(TaskFunction, std::vector<ObjectType>)> ReceiveSome(TaskFunction&& taskFunction)
        {
            auto bypassFlag = std::shared_ptr<TaskBypassFlag>(new TaskBypassFlag({ false }));
            auto observableQueue = m_observableQueue;

            auto func = [observableQueue, taskFunction, bypassFlag]() {
                std::vector<ObjectType> objQueue;
                while (true)
                {
                    auto ret = observableQueue->PopSome(objQueue);
                    if (ret.IsSuccess())
                        break;

                    // if the queue is empty, and closed,
                    // set the Bypass flag, and the thread function will run to exit
                    if (ret.IsClosed())
                    {
                        bypassFlag->Bypass = true;
                        return FUNCTION_WITH_ARGUMENT_RETURN_TYPE(TaskFunction, std::vector<ObjectType>)();
                    }
                }
                return taskFunction(objQueue);
            };
            auto taskDetails = std::make_shared<TaskDetails<FUNCTION_WITH_ARGUMENT_RETURN_TYPE(TaskFunction, std::vector<ObjectType>)> >(
                std::forward<std::function<FUNCTION_WITH_ARGUMENT_RETURN_TYPE(TaskFunction, std::vector<ObjectType>)()> >(func),
                bypassFlag
            );
            return ObserveTask<FUNCTION_WITH_ARGUMENT_RETURN_TYPE(TaskFunction, std::vector<ObjectType>)>(taskDetails);
        }

    private:
        std::shared_ptr<ObservableQueue<ObjectType> > m_observableQueue;
    };

    /////////////////////////////////////////////////
    /// function Observe
    /////////////////////////////////////////////////
    template<typename ObjectType>
    Observable<ObjectType> Observe(std::shared_ptr<ObservableQueue<ObjectType> > observableQueue)
    {
        return Observable<ObjectType>(observableQueue);
    }
}
