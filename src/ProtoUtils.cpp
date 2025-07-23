#include "jungi/mobilus_gtw_client/ProtoUtils.h"
#include "jungi/mobilus_gtw_client/EventNumber.h"
#include "jungi/mobilus_gtw_client/MessageType.h"
#include "jungi/mobilus_gtw_client/proto/CallEvents.pb.h"
#include "jungi/mobilus_gtw_client/proto/CurrentStateRequest.pb.h"
#include "jungi/mobilus_gtw_client/proto/CurrentStateResponse.pb.h"
#include "jungi/mobilus_gtw_client/proto/DeviceSettingsRequest.pb.h"
#include "jungi/mobilus_gtw_client/proto/DeviceSettingsResponse.pb.h"
#include "jungi/mobilus_gtw_client/proto/DevicesListRequest.pb.h"
#include "jungi/mobilus_gtw_client/proto/DevicesListResponse.pb.h"
#include "jungi/mobilus_gtw_client/proto/LoginRequest.pb.h"
#include "jungi/mobilus_gtw_client/proto/LoginResponse.pb.h"
#include "jungi/mobilus_gtw_client/proto/NetworkSettingsRequest.pb.h"
#include "jungi/mobilus_gtw_client/proto/NetworkSettingsResponse.pb.h"

namespace jungi::mobilus_gtw_client {

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
