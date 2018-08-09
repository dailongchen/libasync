#pragma once

#include "NotifyDetails.h"

namespace Async {

    /////////////////////////////////////////////////
    /// function Notify
    /////////////////////////////////////////////////
    template<typename NotifyData>
    void Notify(NotifyData data)
    {
        auto func = GetNotifyFunction<typename std::remove_const<NotifyData>::type>();
        if (func)
            func(data);
    }
}