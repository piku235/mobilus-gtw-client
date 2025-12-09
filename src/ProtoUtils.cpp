#include "jungi/mobgtw/ProtoUtils.h"
#include "jungi/mobgtw/EventNumber.h"
#include "jungi/mobgtw/MessageType.h"
#include "jungi/mobgtw/proto/CallEvents.pb.h"
#include "jungi/mobgtw/proto/CurrentStateRequest.pb.h"
#include "jungi/mobgtw/proto/CurrentStateResponse.pb.h"
#include "jungi/mobgtw/proto/DeviceSettingsRequest.pb.h"
#include "jungi/mobgtw/proto/DeviceSettingsResponse.pb.h"
#include "jungi/mobgtw/proto/DevicesListRequest.pb.h"
#include "jungi/mobgtw/proto/DevicesListResponse.pb.h"
#include "jungi/mobgtw/proto/LoginRequest.pb.h"
#include "jungi/mobgtw/proto/LoginResponse.pb.h"
#include "jungi/mobgtw/proto/NetworkSettingsRequest.pb.h"
#include "jungi/mobgtw/proto/NetworkSettingsResponse.pb.h"
#include "jungi/mobgtw/proto/UpdateDeviceRequest.pb.h"
#include "jungi/mobgtw/proto/UpdateDeviceResponse.pb.h"

namespace jungi::mobgtw {

uint8_t ProtoUtils::messageTypeFor(const google::protobuf::MessageLite& message)
{
    if (!message.GetTypeName().compare(PACKAGE "LoginRequest")) {
        return MessageType::LoginRequest;
    }
    if (!message.GetTypeName().compare(PACKAGE "LoginResponse")) {
        return MessageType::LoginResponse;
    }
    if (!message.GetTypeName().compare(PACKAGE "DevicesListRequest")) {
        return MessageType::DevicesListRequest;
    }
    if (!message.GetTypeName().compare(PACKAGE "DevicesListResponse")) {
        return MessageType::DevicesListResponse;
    }
    if (!message.GetTypeName().compare(PACKAGE "DeviceSettingsRequest")) {
        return MessageType::DeviceSettingsRequest;
    }
    if (!message.GetTypeName().compare(PACKAGE "DeviceSettingsResponse")) {
        return MessageType::DeviceSettingsResponse;
    }
    if (!message.GetTypeName().compare(PACKAGE "CallEvents")) {
        return MessageType::CallEvents;
    }
    if (!message.GetTypeName().compare(PACKAGE "UpdateDeviceRequest")) {
        return MessageType::UpdateDeviceRequest;
    }
    if (!message.GetTypeName().compare(PACKAGE "UpdateDeviceResponse")) {
        return MessageType::UpdateDeviceResponse;
    }
    if (!message.GetTypeName().compare(PACKAGE "CurrentStateRequest")) {
        return MessageType::CurrentStateRequest;
    }
    if (!message.GetTypeName().compare(PACKAGE "CurrentStateResponse")) {
        return MessageType::CurrentStateResponse;
    }
    if (!message.GetTypeName().compare(PACKAGE "NetworkSettingsRequest")) {
        return MessageType::NetworkSettingsRequest;
    }
    if (!message.GetTypeName().compare(PACKAGE "NetworkSettingsResponse")) {
        return MessageType::NetworkSettingsResponse;
    }

    return MessageType::Unknown;
}

std::unique_ptr<google::protobuf::MessageLite> ProtoUtils::newMessageFor(uint8_t messageType)
{
    switch (messageType) {
    case MessageType::LoginRequest:
        return std::make_unique<proto::LoginRequest>();
    case MessageType::LoginResponse:
        return std::make_unique<proto::LoginResponse>();
    case MessageType::DevicesListRequest:
        return std::make_unique<proto::DevicesListRequest>();
    case MessageType::DevicesListResponse:
        return std::make_unique<proto::DevicesListResponse>();
    case MessageType::DeviceSettingsRequest:
        return std::make_unique<proto::DeviceSettingsRequest>();
    case MessageType::DeviceSettingsResponse:
        return std::make_unique<proto::DeviceSettingsResponse>();
    case MessageType::CallEvents:
        return std::make_unique<proto::CallEvents>();
    case MessageType::UpdateDeviceRequest:
        return std::make_unique<proto::UpdateDeviceRequest>();
    case MessageType::UpdateDeviceResponse:
        return std::make_unique<proto::UpdateDeviceResponse>();
    case MessageType::CurrentStateRequest:
        return std::make_unique<proto::CurrentStateRequest>();
    case MessageType::CurrentStateResponse:
        return std::make_unique<proto::CurrentStateResponse>();
    case MessageType::NetworkSettingsRequest:
        return std::make_unique<proto::NetworkSettingsRequest>();
    case MessageType::NetworkSettingsResponse:
        return std::make_unique<proto::NetworkSettingsResponse>();
    default:
        return nullptr;
    }
}

}
