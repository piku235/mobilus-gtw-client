#include "jungi/mobgtw/MqttMobilusGtwClient.h"
#include "MqttMobilusGtwClientImpl.h"

namespace jungi::mobgtw {

MqttMobilusGtwClient::Builder& MqttMobilusGtwClient::Builder::dsn(MqttDsn dsn)
{
    mDsn.emplace(std::move(dsn));
    return *this;
}

MqttMobilusGtwClient::Builder& MqttMobilusGtwClient::Builder::login(MobilusCredentials mobilusCredentials)
{
    mMobilusCredentials.emplace(std::move(mobilusCredentials));
    return *this;
}

MqttMobilusGtwClient::Builder& MqttMobilusGtwClient::Builder::attachTo(io::EventLoop* loop)
{
    mLoop = loop;
    return *this;
}

MqttMobilusGtwClient::Builder& MqttMobilusGtwClient::Builder::useLogger(logging::Logger* logger)
{
    mLogger = logger;
    return *this;
}

MqttMobilusGtwClient::Builder& MqttMobilusGtwClient::Builder::useKeepAliveMessage(std::unique_ptr<google::protobuf::MessageLite> message)
{
    mKeepAliveMessage = std::move(message);
    return *this;
}

MqttMobilusGtwClient::Builder& MqttMobilusGtwClient::Builder::connectTimeout(std::chrono::milliseconds timeout)
{
    mConnectTimeout = timeout;
    return *this;
}

MqttMobilusGtwClient::Builder& MqttMobilusGtwClient::Builder::responseTimeout(std::chrono::milliseconds timeout)
{
    mResponseTimeout = timeout;
    return *this;
}

MqttMobilusGtwClient::Builder& MqttMobilusGtwClient::Builder::onSessionExpiring(SessionExpiringCallback callback)
{
    mSessionExpiringCallback.emplace(std::move(callback));
    return *this;
}

MqttMobilusGtwClient::Builder& MqttMobilusGtwClient::Builder::onRawMessage(RawMessageCallback callback)
{
    mRawMessageCallback.emplace(std::move(callback));
    return *this;
}

std::unique_ptr<MqttMobilusGtwClient> MqttMobilusGtwClient::Builder::build()
{
    if (!mDsn || !mMobilusCredentials) {
        return nullptr;
    }

    auto client = std::make_unique<MqttMobilusGtwClientImpl>(*mDsn, *mMobilusCredentials, mConnectTimeout, mResponseTimeout, *mLoop, *mLogger);

    if (mKeepAliveMessage) {
        client->useKeepAliveMessage(std::move(mKeepAliveMessage));
    }
    if (mSessionExpiringCallback) {
        client->onSessionExpiring(std::move(*mSessionExpiringCallback));
    }
    if (mRawMessageCallback) {
        client->onRawMessage(std::move(*mRawMessageCallback));
    }

    return client;
}

std::unique_ptr<MqttMobilusGtwClient> MqttMobilusGtwClient::from(MqttDsn dsn, MobilusCredentials mobilusCredentials)
{
    return builder()
        .dsn(std::move(dsn))
        .login(std::move(mobilusCredentials))
        .build();
}

MqttMobilusGtwClient::Builder MqttMobilusGtwClient::builder()
{
    return {};
}

}
