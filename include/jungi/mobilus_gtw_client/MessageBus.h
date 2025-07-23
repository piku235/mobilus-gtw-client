#pragma once

#include "ProtoUtils.h"
#include <google/protobuf/message_lite.h>

#include <cstdint>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace jungi::mobilus_gtw_client {

static constexpr uint8_t kMessageTypeAll = 0;

class MessageBus final {
public:
    void subscribeAll(std::function<void(const google::protobuf::MessageLite&)> subscriber)
    {
        subscribe(kMessageTypeAll, subscriber);
    }

    template <class T, class = std::enable_if_t<std::is_base_of_v<google::protobuf::MessageLite, T>>>
    void subscribe(uint8_t messageType, std::function<void(const T&)> subscriber)
    {
        auto& subscribers = mSubscribers[messageType];
        subscribers.push_back([subscriber = std::move(subscriber)](auto& message) { subscriber(static_cast<const T&>(message)); });
    }

    void dispatch(const google::protobuf::MessageLite& message)
    {
        dispatchFor(ProtoUtils::messageTypeFor(message), message); // direct
        dispatchFor(kMessageTypeAll, message); // all
    }

private:
    using MessageSubscriber = std::function<void(const google::protobuf::MessageLite&)>;

    std::unordered_map<uint8_t, std::vector<MessageSubscriber>> mSubscribers;

    void dispatchFor(uint8_t messageType, const google::protobuf::MessageLite& message)
    {
        auto& subscribers = mSubscribers[messageType];

        for (auto& subscriber: subscribers) {
            subscriber(message);
        }
    }
};

}
