#pragma once

#include "CancelDetails.h"

namespace Async {

    class Cancel
    {
    public:
        /////////////////////////////////////////////////
        /// function IsCancelled
        /////////////////////////////////////////////////
        static bool IsCancelled()
        {
            if (*(CancelTrigger::GetCancelTrigger()))
                return (*(CancelTrigger::GetCancelTrigger()))->Get();

            // if no cancelTrigger, then always return not-cancelled
            return false;
        }

        /////////////////////////////////////////////////
        /// function CancelCurrentTask
        /////////////////////////////////////////////////
        static void CancelCurrentTask()
        {
            if (*(CancelTrigger::GetCancelTrigger()))
                (*(CancelTrigger::GetCancelTrigger()))->Set(true);
        }
    };
}
