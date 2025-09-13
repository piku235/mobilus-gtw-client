#include "jungi/mobilus_gtw_client/MqttDsn.h"

#include <regex>
#include <unordered_map>

namespace jungi::mobilus_gtw_client {

std::optional<MqttDsn> MqttDsn::from(const std::string& dsn)
{
    std::regex uri(R"(^(mqtt(?:s)?)://(?:(.+?):(.+?)@)?([^:/?]+)(?::(\d+))?(?:\?([^#]+))?$)");
    std::smatch umatches;

    if (!std::regex_match(dsn, umatches, uri)) {
        return std::nullopt;
    }

    auto queryString = umatches[6].str();

    std::regex query(R"((\w+)=([^&]+))");
    std::unordered_map<std::string, std::string> queryParams;
    std::smatch qmatches;

    std::string::const_iterator start(queryString.cbegin());
    while (std::regex_search(start, queryString.cend(), qmatches, query)) {
        queryParams[qmatches[1]] = qmatches[2];
        start = qmatches.suffix().first;
    }

    std::optional<std::string> caFile;

    {
        auto r = queryParams.find("ca_file");
        if (r != queryParams.end()) {
            caFile = r->second;
        }
    }

    return MqttDsn(
        umatches[4].str(),
        umatches[5].str().empty() ? std::nullopt : std::optional(static_cast<uint16_t>(std::stoi(umatches[5].str()))),
        umatches[2].str().empty() ? std::nullopt : std::optional(umatches[2].str()),
        umatches[3].str().empty() ? std::nullopt : std::optional(umatches[3].str()),
        "mqtts" == umatches[1].str(),
        caFile);
}

MqttDsn::MqttDsn(std::string host, std::optional<uint16_t> port, std::optional<std::string> username, std::optional<std::string> password, bool secure, std::optional<std::string> caFile)
    : mHost(std::move(host))
    , mPort(std::move(port))
    , mUsername(std::move(username))
    , mPassword(std::move(password))
    , mSecure(secure)
    , mCaFile(std::move(caFile))
{
}

}
