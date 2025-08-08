#include "jungi/mobilus_gtw_client/Envelope.h"
#include "jungi/mobilus_gtw_client/MessageType.h"
#include "jungi/mobilus_gtw_client/Platform.h"

#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <cstdint>
#include <vector>

using namespace jungi::mobilus_gtw_client;

Envelope envelopeStub()
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

TEST(EnvelopeTest, Equals)
{
    EXPECT_EQ(envelopeStub(), envelopeStub());
}

TEST(EnvelopeTest, DoesNotEqual)
{
    Envelope envelope = envelopeStub();
    std::vector<Envelope> otherEnvelops = {
        Envelope { MessageType::CallEvents },
        Envelope { MessageType::CallEvents, 1754566797, { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 }, Platform::Host, Envelope::ResponseStatus::Success, { 0x12, 0x13, 0x14 } },
        Envelope { MessageType::CallEvents, 1754566797, { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 }, Platform::Web, Envelope::ResponseStatus::AuthenticationFailed, { 0x12, 0x13, 0x14 } },
        Envelope { MessageType::CallEvents, 1754566797, { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 }, Platform::Web, Envelope::ResponseStatus::Success, { 0x12, 0x13, 0x15 } }
    };

    for (auto& other : otherEnvelops) {
        EXPECT_FALSE(envelope == other);
        EXPECT_FALSE(other == envelope);
    }
}

TEST(EnvelopeTest, SizeForEmptyEnvelope)
{
    Envelope envelope;

    EXPECT_EQ(13, envelope.size());
}

TEST(EnvelopeTest, SizeForEnvelopeWithBody)
{
    Envelope envelope = envelopeStub();

    EXPECT_EQ(16, envelope.size());
}

TEST(EnvelopeTest, SerializesAndDeserializes)
{
    Envelope envelope = envelopeStub();

    auto serialized = envelope.serialize();
    auto deserialized = Envelope::deserialize(serialized.data(), serialized.size());

    EXPECT_EQ(envelope.size() + sizeof(uint32_t), serialized.size()); // sizeof(uint32_t) - extra space for message size
    EXPECT_TRUE(deserialized.has_value());
    EXPECT_EQ(envelope, *deserialized);
}

TEST(EnvelopeTest, DeserializeFailsForTooSmallPayload)
{
    uint8_t payload[] = { 0x11, 0x12, 0x13 };
    auto deserialized = Envelope::deserialize(payload, sizeof(payload));

    EXPECT_FALSE(deserialized.has_value());
}

TEST(DEnvelopeTest, eserializeFailsDueToSizeMismatch)
{
    uint8_t payload[sizeof(uint32_t)];
    uint32_t messageSize = htonl(123);
    memcpy(payload, &messageSize, sizeof(messageSize));

    auto deserialized = Envelope::deserialize(payload, sizeof(payload));

    EXPECT_FALSE(deserialized.has_value());
}
