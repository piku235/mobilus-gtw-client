#pragma once

#include "ClientCommonCommand.h"

#include <google/protobuf/repeated_field.h>
#include <cxxopts.hpp>

namespace mobcli::commands {

class DeviceListCommand final : public ClientCommonCommand {
public:
    DeviceListCommand();

    int execute(int argc, char* argv[]) override;
    std::string_view name() const override { return "device-list"; }
    std::string_view description() const override { return "makes device list query and prints the response"; }

private:
    cxxopts::Options mOpts;

    template <typename T>
    std::string formatList(const google::protobuf::RepeatedField<T>& repeatedField);
};

}
