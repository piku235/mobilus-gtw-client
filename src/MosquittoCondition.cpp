#include "MosquittoCondition.h"

#include <ctime>

namespace jungi::mobilus_gtw_client {

static constexpr uint16_t kWaitTime = 10; // seconds

MosquittoCondition::MosquittoCondition(mosquitto* mosq)
    : mMosq(mosq)
{
}

void MosquittoCondition::notify()
{
    mCondition = true;
}

void MosquittoCondition::wait()
{
    auto untilTime = time(nullptr) + kWaitTime;
    int rc;

    while (!mCondition && time(nullptr) < untilTime) {
        rc = mosquitto_loop(mMosq, -1, 1);
        if (MOSQ_ERR_SUCCESS != rc) {
            break;
        }
    }

    mCondition = false;
}

}
