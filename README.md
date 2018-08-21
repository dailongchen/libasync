# libasync, a header only async library in C++11

### Intention

* Inspired by
  - Reactive Extensions (Microsoft)
  - Async++::Task
* Following Functional Reactive Programming
  * Event-Reactive mode
  * Design Module to be as simple as possible
  * One Event Stream contains several steps, each step should have quite few logic
  * Event Stream could have one or several steps, but try to avoid reusing Event Stream for different functions
  * Let the Async Library take care of asynchrony, Event Stream creation and sub-thread life cycle

### Components

* Task
  * Usages: To run a task in async, and fire the trigger of specific event/result handler.
  * Functions: Spawn, Get, Notified, OnException, Run, Cancel
* ObserveTask
  * Usage: To observe any "add" event of ObservableQueue, and trigger specific event handler.
  * Functions: Observe, Notified, OnException, Run, Cancel
* ObservableQueue
  * Usage: A queue container which can be observed by ObserveTask.

### Usages

Refer to test_asynctask.cpp

### Example

```cpp
auto task = Async::Spawn([] {
    Async::Notify(std::string("I'm here"));
    return 110;
}).Then([] { // Then will ignore the previous step return value
    Async::Notify(1122);
    return "abcd";
}).Get([](const std::string& i) {
    return 10.1;
}).Get([](double i) {
    std::string().at(1); // test GetException
    return float(1.2);
}).OnException([](std::exception_ptr exceptionPtr) {
    try{
        std::rethrow_exception(exceptionPtr);
    }
    catch (const std::exception&) {
    }
}).Notified<std::string>([](const std::string& a) {
    // string type Async::Notify will go here
}).Notified<int>([](int a) {
    // int type Async::Notify will go here
}).OnBegin([] {
    // at beginning of task
}).OnEnd([] {
    // at the end of task
});

// by default it should run in async mode,
// but you can force it in sync mode.
task.Run(Async::RunMode::RunMode_Sync);
```
