#include "MockMqttMobilusActor.h"
#include "jungi/mobilus_gtw_client/io/SocketEvents.h"

#include <condition_variable>
#include <cstdio>
#include <mutex>

namespace jungi::mobilus_gtw_client::tests::mocks {

MockMqttMobilusActor::MockMqttMobilusActor(std::string host, size_t port)
    : mHost(std::move(host))
    , mPort(port)
{
}

MockMqttMobilusActor::~MockMqttMobilusActor()
{
    stop();
}

void MockMqttMobilusActor::mockResponseFor(uint8_t requestType, std::unique_ptr<const google::protobuf::MessageLite> response)
{
    mMockResponses.emplace(requestType, std::move(response));
}

void MockMqttMobilusActor::run()
{
    if (isRunning()) {
        return;
    }

    std::mutex mutex;
    std::condition_variable cv;
    bool ready = false;

    mThread = std::thread([&]() {
        MockMqttMobilusService mobilusService(mHost, mPort, std::move(mMockResponses), mSocketWatcher);

        {
            std::unique_lock<std::mutex> lock(mutex);

            auto r = mobilusService.connect();

            ready = true;
            lock.unlock();
            cv.notify_one();

            if (!r) {
                return;
            }
        }

        mSocketWatcher.loopFor(std::chrono::seconds(1));
    });

    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&]() -> bool { return ready; });
}

void MockMqttMobilusActor::stop()
{
    if (!isRunning()) {
        return;
    }

    mSocketWatcher.stop();
    
    if (mThread.joinable()) {
        mThread.join();
    }
}

bool MockMqttMobilusActor::isRunning() const
{
    return std::thread::id() != mThread.get_id();
}

}
