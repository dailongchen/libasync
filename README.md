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