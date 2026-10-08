#pragma once

#include <Arduino.h>

enum class RadioPacketType : uint8_t {
  Unknown = 0,
  Heartbeat = 1,
  Status = 2,
  Command = 3,
  Acknowledgement = 4
};

enum class RadioNode : uint8_t {
  Unknown = 0,
  NodeV1 = 1,
  Piku21RevA = 2
};

struct __attribute__((packed)) RadioPacket {
  uint8_t protocolVersion = 1;
  uint8_t packetType = static_cast<uint8_t>(RadioPacketType::Unknown);
  uint8_t sourceNode = static_cast<uint8_t>(RadioNode::Unknown);
  uint8_t flags = 0;
  uint16_t sequence = 0;
  uint8_t alarmMask = 0;
  uint8_t reserved = 0;
  uint32_t uptimeMs = 0;
};

static_assert(sizeof(RadioPacket) == 12, "RadioPacket layout changed");
static_assert(sizeof(RadioPacket) <= 32, "nRF24 payload exceeds 32 bytes");

struct RadioSnapshot {
  bool initialized = false;
  bool chipDetected = false;
  bool listening = false;
  uint8_t channel = 0;
  uint32_t txAttempts = 0;
  uint32_t txSuccess = 0;
  uint32_t txFailure = 0;
  uint32_t consecutiveTxFailures = 0;
  uint32_t rxPackets = 0;
  uint32_t lastTxMs = 0;
  uint32_t lastTxSuccessMs = 0;
  uint32_t lastRxMs = 0;
  bool peerEverSeen = false;
  uint32_t lastPeerSeenMs = 0;
  uint8_t lastPacketType = static_cast<uint8_t>(RadioPacketType::Unknown);
  uint8_t lastSourceNode = static_cast<uint8_t>(RadioNode::Unknown);
};
