#to define ${SRCS_ASYNC}, and export to parent scope

set(SRCS_ASYNC
    async/Async.h
    async/details/Cancel.h
    async/details/CancelDetails.h
    async/details/ExceptionDetails.h
    async/details/Notify.h
    async/details/NotifyDetails.h
    async/details/Observe.h
    async/details/Task.h
    async/details/TaskDetails.h
    async/details/TaskHandle.h
    async/details/ThreadLocal.h
)

set(SRCS_ASYNC
    ${SRCS_ASYNC}
    PARENT_SCOPE
)