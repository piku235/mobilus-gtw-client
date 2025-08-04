#include "jungi/mobilus_gtw_client/Envelope.h"

#include <arpa/inet.h>
#include <cstring>

namespace jungi::mobilus_gtw_client {

uint32_t Envelope::size() const
{
    return static_cast<uint32_t>(
        sizeof(messageType)
        + sizeof(timestamp)
        + clientId.size()
        + sizeof(platform)
        + sizeof(responseStatus)
        + messageBody.size()
    );
}

std::optional<Envelope> Envelope::deserialize(const uint8_t* payload, uint32_t size)
{
    Envelope envelope;

    // is lower than min size
    if (size < envelope.size()) {
        return std::nullopt;
    }

    uint32_t messageSize;

    const uint8_t* offset = payload;

    memcpy(&messageSize, offset, sizeof(messageSize));
    messageSize = ntohl(messageSize);
    offset += sizeof(messageSize);

    // size mismtach
    if (messageSize + sizeof(messageSize) != size) {
        return std::nullopt;
    }

    memcpy(&envelope.messageType, offset, sizeof(envelope.messageType));
    offset += sizeof(envelope.messageType);

    memcpy(&envelope.timestamp, offset, sizeof(envelope.timestamp));
    envelope.timestamp = ntohl(envelope.timestamp);
    offset += sizeof(envelope.timestamp);

    memcpy(envelope.clientId.data(), offset, envelope.clientId.size());
    offset += envelope.clientId.size();

    memcpy(&envelope.platform, offset, sizeof(envelope.platform));
    offset += sizeof(envelope.platform);

    memcpy(&envelope.responseStatus, offset, sizeof(envelope.responseStatus));
    offset += sizeof(envelope.responseStatus);

    envelope.messageBody.resize(messageSize - envelope.size());
    memcpy(envelope.messageBody.data(), offset, envelope.messageBody.size());
    offset += envelope.messageBody.size();

    return envelope;
}

std::vector<uint8_t> Envelope::serialize() const
{
    auto aSize = size();
    std::vector<uint8_t> payload(aSize + sizeof(aSize));
    uint8_t* offset = payload.data();

    auto nSize = htonl(static_cast<uint32_t>(aSize));
    memcpy(offset, &nSize, sizeof(nSize));
    offset += sizeof(nSize);

    memcpy(offset, &messageType, sizeof(messageType));
    offset += sizeof(messageType);

    uint32_t aTimestamp = htonl(timestamp);
    memcpy(offset, &aTimestamp, sizeof(aTimestamp));
    offset += sizeof(aTimestamp);

    memcpy(offset, clientId.data(), clientId.size());
    offset += clientId.size();

    memcpy(offset, &platform, sizeof(platform));
    offset += sizeof(platform);

    memcpy(offset, &responseStatus, sizeof(responseStatus));
    offset += sizeof(responseStatus);

    memcpy(offset, messageBody.data(), messageBody.size());
    offset += messageBody.size();

    return payload;
}

}
