#include <string>
#include <iostream>
#include <sstream>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/test/unit_test.hpp>

#include "Async.h"

struct TestAsyncLibraryFixture {
    ~TestAsyncLibraryFixture() {
    }
};

BOOST_FIXTURE_TEST_SUITE(TestAsyncLibrary, TestAsyncLibraryFixture)

BOOST_AUTO_TEST_CASE(TestAsyncTask) {
    // test Async::Task
    std::vector<std::string> expectResults{
        "OnBegin",
        "I'm here",
        "Spawn",
        "1122",
        "Then",
        "Get abcd",
        "Get 10.100000",
        "OnException",
        "OnEnd"
    };
    std::vector<std::string> testResults;

    auto t1 = Async::Spawn([&testResults] {
        Async::Notify(std::string("I'm here"));
        testResults.push_back("Spawn");
        return 110;
    }).Then([&testResults] {
        Async::Notify(1122);
        testResults.push_back("Then");
        return "abcd";
    }).Get([&testResults](const std::string& i) {
        testResults.push_back("Get " + i);
        return 10.1;
    }).Get([&testResults](double i) {
        testResults.push_back("Get " + std::to_string(i));
        std::string().at(1); // test GetException
        return float(1.2);
    }).OnException([&testResults](std::exception_ptr exceptionPtr) {
        try{
            std::rethrow_exception(exceptionPtr);
        }
        catch (const std::exception&) {
            testResults.push_back("OnException");
        }
    }).Notified<std::string>([&testResults](const std::string& a) {
        testResults.push_back(a);
    }).Notified<int>([&testResults](int a) {
        testResults.push_back(std::to_string(a));
    }).OnBegin([&testResults] {
        testResults.push_back("OnBegin");
    }).OnEnd([&testResults] {
        testResults.push_back("OnEnd");
    });

    t1.Run(Async::RunMode::RunMode_Sync);

    BOOST_REQUIRE(expectResults == testResults);
}

BOOST_AUTO_TEST_CASE(TestAsyncTask_WithoutThenGet) {
    // test Async::Task without Then/Get
    std::vector<std::string> expectResults{
        "Spawn"
    };
    std::vector<std::string> testResults;

    auto t1 = Async::Spawn([&testResults] {
        Async::Notify(std::string("I'm here"));
        testResults.push_back("Spawn");
        return 110;
    });

    auto taskHandle = t1.Run(Async::RunMode::RunMode_Async);
    taskHandle->Join();

    BOOST_REQUIRE(expectResults == testResults);
}

BOOST_AUTO_TEST_CASE(TestAsyncObserveTaskReceiveOne) {
    // test Async::ObserveTask::ReceiveOne
    std::vector<std::string> expectResults{
        "OnBegin",
        "ReceiveOne: 1",
        "ReceiveOne-Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: 2",
        "ReceiveOne-Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: 3",
        "ReceiveOne-Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: abcd",
        "ReceiveOne-Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: 1",
        "ReceiveOne-Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: 2",
        "ReceiveOne-Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: 3",
        "ReceiveOne-Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: 4",
        "ReceiveOne-Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: efgh",
        "ReceiveOne-Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: ijkl",
        "ReceiveOne-Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: mnop",
        "ReceiveOne-Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: qrst",
        "ReceiveOne-Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: uvwxyz",
        "ReceiveOne-Get: wa-ka-wa-ka",
        "OnException",
        "OnEnd"
    };
    std::vector<std::string> testResults;

    auto queue = Async::ObservableQueue<std::string>::New();
    queue->PushSome(std::vector<std::string>({ "1", "2" }));
    queue->PushOne("3");

    auto a1 = Async::Observe(
        queue
    ).ReceiveOne([&testResults](const std::string& k) {
        testResults.push_back("ReceiveOne: " + k);
    }).Then([] {
        return "wa-ka-wa-ka";
    }).Get([&testResults](const std::string& k) {
        testResults.push_back("ReceiveOne-Get: " + k);
    }).Then([] {
        std::string().at(1); // test GetException
    }).OnException([&testResults](std::exception_ptr exceptionPtr) {
        try{
            std::rethrow_exception(exceptionPtr);
        }
        catch (const std::exception&) {
            testResults.push_back("OnException");
        }
    }).Notified<std::string>([&testResults](const std::string& a) {
        testResults.push_back(a);
    }).Notified<int>([&testResults](int a) {
        testResults.push_back(std::to_string(a));
    }).OnBegin([&testResults] {
        testResults.push_back("OnBegin");
    }).OnEnd([&testResults] {
        testResults.push_back("OnEnd");
    });

    auto taskHandle = a1.Run();

    queue->PushOne(std::string("abcd"));
    std::this_thread::sleep_for(std::chrono::seconds(5));
    queue->PushSome(std::vector<std::string>({ "1", "2", "3", "4" }));
    queue->PushOne(std::string("efgh"));
    queue->PushOne(std::string("ijkl"));
    queue->PushOne(std::string("mnop"));
    queue->PushOne(std::string("qrst"));
    queue->PushOne(std::string("uvwxyz"));

    // Join without Cancel will let the task consume all rest objects in queue before exit
    //taskHandle->Cancel();
    queue->Close();
    taskHandle->Join();

    BOOST_REQUIRE(expectResults == testResults);
}

BOOST_AUTO_TEST_CASE(TestAsyncObserveTaskReceiveSome) {
    // test Async::ObserveTask::ReceiveSome
    std::vector<std::string> expectResults{
        "ReceiveSome: ",
        "ReceiveSome-Notified",
        "ReceiveSome-Get: wa-ka-wa-ka",
        "OnException"
    };
    std::vector<std::string> testResults;

    auto queue = Async::ObservableQueue<std::string>::New();
    auto a1 = Async::Observe(
        queue
    ).ReceiveSome([&testResults](const std::vector<std::string>& k) {
        std::stringstream ss;
        std::copy(k.begin(), k.end(), std::ostream_iterator<std::string>(ss, " "));
        testResults.push_back("ReceiveSome: " + ss.str());
        Async::Notify(std::string("ReceiveSome-Notified"));
    }).Then([] {
        return "wa-ka-wa-ka";
    }).Get([&testResults](const std::string& k) {
        testResults.push_back("ReceiveSome-Get: " + k);
    }).Then([] {
        std::string().at(1); // test GetException
    }).OnException([&testResults](std::exception_ptr exceptionPtr) {
        try{
            std::rethrow_exception(exceptionPtr);
        }
        catch (const std::exception&) {
            testResults.push_back("OnException");
        }
    }).Notified<std::string>([&testResults](const std::string& a) {
        testResults.push_back(a);
    }).Notified<int>([&testResults](int a) {
        testResults.push_back(std::to_string(a));
    });

    auto taskHandle = a1.Run();

    queue->PushOne(std::string("abcd"));
    std::this_thread::sleep_for(std::chrono::seconds(5));
    queue->PushSome(std::vector<std::string>({ "1", "2", "3", "4" }));
    queue->PushOne(std::string("efgh"));
    queue->PushOne(std::string("ijkl"));
    queue->PushOne(std::string("mnop"));
    queue->PushOne(std::string("qrst"));
    queue->PushOne(std::string("uvwxyz"));

    // comment this line out will let the task consume all rest objects in queue before exit
    taskHandle->Cancel();
    taskHandle->Join();

    BOOST_REQUIRE(!testResults.empty());
    BOOST_REQUIRE_EQUAL(testResults.size() % expectResults.size(), 0);

    const size_t steps = expectResults.size();
    const size_t receiveTimes = testResults.size() / steps;
    for (size_t i = 0; i < receiveTimes; i++)
    {
        BOOST_REQUIRE(boost::starts_with(testResults[i * steps], expectResults[0]));
        BOOST_REQUIRE_EQUAL(testResults[i * steps + 1], expectResults[1]);
        BOOST_REQUIRE_EQUAL(testResults[i * steps + 2], expectResults[2]);
        BOOST_REQUIRE_EQUAL(testResults[i * steps + 3], expectResults[3]);
    }
}

BOOST_AUTO_TEST_CASE(TestAsyncObserveQueueClose) {
    // test Async::ObserveTask::ReceiveOne
    std::vector<std::string> expectResults{
        "OnBegin",
        "ReceiveOne: 1",
        "ReceiveOne-Notified",
        "Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: 2",
        "ReceiveOne-Notified",
        "Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: 3",
        "ReceiveOne-Notified",
        "Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: abcd",
        "ReceiveOne-Notified",
        "Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: 1",
        "ReceiveOne-Notified",
        "Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: 2",
        "ReceiveOne-Notified",
        "Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: 3",
        "ReceiveOne-Notified",
        "Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: 4",
        "ReceiveOne-Notified",
        "Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: efgh",
        "ReceiveOne-Notified",
        "Get: wa-ka-wa-ka",
        "OnException",
        "ReceiveOne: ijkl",
        "ReceiveOne-Notified",
        "Get: wa-ka-wa-ka",
        "OnException",
        "OnEnd"
    };
    std::vector<std::string> testResults;

    {
        auto queue = Async::ObservableQueue<std::string>::New();
        queue->PushSome(std::vector<std::string>({ "1", "2" }));
        queue->PushOne("3");

        auto a1 = Async::Observe(
            queue
        ).ReceiveOne([&testResults](const std::string& k) {
            testResults.push_back("ReceiveOne: " + k);
            Async::Notify(std::string("ReceiveOne-Notified"));
        }).Then([] {
            return "wa-ka-wa-ka";
        }).Get([&testResults](const std::string& k) {
            testResults.push_back("Get: " + k);
        }).Then([] {
            std::string().at(1); // test GetException
        }).OnException([&testResults](std::exception_ptr exceptionPtr) {
            try{
                std::rethrow_exception(exceptionPtr);
            }
            catch (const std::exception&) {
                testResults.push_back("OnException");
            }
        }).Notified<std::string>([&testResults](const std::string& a) {
            testResults.push_back(a);
        }).Notified<int>([&testResults](int a) {
            testResults.push_back(std::to_string(a));
        }).OnBegin([&testResults] {
            testResults.push_back("OnBegin");
        }).OnEnd([&testResults] {
            testResults.push_back("OnEnd");
        });

        a1.Run();

        queue->PushOne(std::string("abcd"));
        std::this_thread::sleep_for(std::chrono::seconds(5));
        queue->PushSome(std::vector<std::string>({ "1", "2", "3", "4" }));
        queue->PushOne(std::string("efgh"));
        queue->PushOne(std::string("ijkl"));
        queue->Close();

        queue->PushOne(std::string("mnop"));
        queue->PushOne(std::string("qrst"));
        queue->PushOne(std::string("uvwxyz"));
    }

    // wait for ObserveTask run to end by itself
    std::this_thread::sleep_for(std::chrono::seconds(5));

    BOOST_REQUIRE(expectResults == testResults);
}

BOOST_AUTO_TEST_SUITE_END()
