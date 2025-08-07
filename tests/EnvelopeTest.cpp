#include "jungi/mobilus_gtw_client/Envelope.h"
#include "jungi/mobilus_gtw_client/MessageType.h"
#include "jungi/mobilus_gtw_client/Platform.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cstdint>
#include <vector>
#include <arpa/inet.h>

using namespace jungi::mobilus_gtw_client;

Envelope makeEnvelope()
{
    return {
        MessageType::CallEvents,
        1754566797,
        { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 },
        Platform::Web,
        Envelope::ResponseStatus::Success,
        { 0x12, 0x13, 0x14 },
    };
}

TEST_CASE("Envelope equals to another", "[unit][Envelope]") {
    REQUIRE(makeEnvelope() == makeEnvelope());
}

TEST_CASE("Envelope does not equal to another", "[unit][Envelope]") {
    Envelope envelope = makeEnvelope();
    Envelope other = GENERATE(
        Envelope { MessageType::CallEvents },
        Envelope { MessageType::CallEvents, 1754566797, { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 }, Platform::Host, Envelope::ResponseStatus::Success, { 0x12, 0x13, 0x14 } },
        Envelope { MessageType::CallEvents, 1754566797, { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 }, Platform::Web, Envelope::ResponseStatus::AuthenticationFailed, { 0x12, 0x13, 0x14 } },
        Envelope { MessageType::CallEvents, 1754566797, { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 }, Platform::Web, Envelope::ResponseStatus::Success, { 0x12, 0x13, 0x15 } }
    );

    REQUIRE_FALSE(envelope == other);
    REQUIRE_FALSE(other == envelope);
}

TEST_CASE("Envelope size is computed", "[unit][Envelope]") {
    SECTION("empty") {
        Envelope envelope;

        REQUIRE(13 == envelope.size());
    }

    SECTION("with body") {
        Envelope envelope = makeEnvelope();
        
        REQUIRE(16 == envelope.size());
    }
}

TEST_CASE("Envelope serializes and deserializes", "[unit][Envelope]") {
    Envelope envelope = makeEnvelope();

    auto serialized = envelope.serialize();
    auto deserialized = Envelope::deserialize(serialized.data(), serialized.size());

    REQUIRE(envelope.size() + sizeof(uint32_t) == serialized.size()); // sizeof(uint32_t) - extra space for message size
    REQUIRE(deserialized.has_value());
    REQUIRE(envelope == *deserialized);
}

TEST_CASE("Envelope deserialize fails for too small payload", "[unit][Envelope]") {
    uint8_t payload[] = { 0x11, 0x12, 0x13 };
    auto deserialized = Envelope::deserialize(payload, sizeof(payload));

    REQUIRE_FALSE(deserialized.has_value());
}

TEST_CASE("Envelope deserialize fails due to size mismatch", "[unit][Envelope]") {
    uint8_t payload[sizeof(uint32_t)];
    uint32_t messageSize = htonl(123);
    memcpy(payload, &messageSize, sizeof(messageSize));

    auto deserialized = Envelope::deserialize(payload, sizeof(payload));

    REQUIRE_FALSE(deserialized.has_value());
}
