#pragma once

#include <mosquitto.h>

namespace jungi::mobilus_gtw_client {

class MosquittoCondition final {
public:
    explicit MosquittoCondition(mosquitto* mosq);

    void notify();
    void wait();

private:
    mosquitto* mMosq;
    bool mCondition = false;
};

}
