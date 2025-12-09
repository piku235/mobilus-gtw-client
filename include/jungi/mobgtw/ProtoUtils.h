#pragma once

#include <google/protobuf/message.h>
#include <memory>

#define PACKAGE "jungi.mobgtw.proto."

namespace jungi::mobgtw {

namespace ProtoUtils {

    uint8_t messageTypeFor(const google::protobuf::MessageLite& message);
    std::unique_ptr<google::protobuf::MessageLite> newMessageFor(uint8_t messageType);

}

}
