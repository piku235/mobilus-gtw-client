#pragma once

#include <cxxopts.hpp>

#include <string_view>

static constexpr char kAppPrefix[] = "mobcli ";

namespace mobcli {

class Command {
public:
    virtual ~Command() = default;
    virtual int execute(int argc, char* argv[]) = 0;
    virtual std::string_view name() const = 0;
    virtual std::string_view description() const = 0;

protected:
    cxxopts::Options makeOptions() const { return cxxopts::Options(std::string(kAppPrefix) + std::string(name()), std::string(description()) + '\n'); };
};

}
