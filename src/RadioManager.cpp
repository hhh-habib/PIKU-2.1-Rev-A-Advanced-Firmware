#include "RadioManager.h"

#include <SPI.h>
#include "PinConfig.h"
#include "RobotConfig.h"

RadioManager::RadioManager()
    : radio_(PinConfig::NRF24_CE, PinConfig::NRF24_CSN, RobotConfig::NRF_SPI_SPEED_HZ) {
  snapshot_.channel = RobotConfig::NRF_CHANNEL;
}

bool RadioManager::begin() {
  if (!RobotConfig::ENABLE_NRF24) {
    Serial.println("[Radio] DISABLED by RobotConfig");
    return false;
  }
  snapshot_.initialized = false;
  snapshot_.chipDetected = false;
  snapshot_.listening = false;
  if (!radio_.begin(&SPI)) {
    Serial.println("[Radio] ERROR: nRF24 not detected; robot services will continue");
    return false;
  }
  snapshot_.chipDetected = radio_.isChipConnected();
  if (!snapshot_.chipDetected) {
    Serial.println("[Radio] ERROR: chip check failed; robot services will continue");
    return false;
  }

  radio_.setAddressWidth(5);
  radio_.setChannel(RobotConfig::NRF_CHANNEL);
  radio_.setDataRate(RF24_250KBPS);
  radio_.setPALevel(RF24_PA_LOW);
  radio_.setAutoAck(true);
  radio_.setCRCLength(RF24_CRC_16);
  radio_.setRetries(RobotConfig::NRF_RETRY_DELAY, RobotConfig::NRF_RETRY_COUNT);
  radio_.enableDynamicPayloads();
  radio_.openWritingPipe(RobotConfig::NRF_NODE_ADDRESS);
  radio_.openReadingPipe(1, RobotConfig::NRF_PIKU_ADDRESS);
  radio_.startListening();
  snapshot_.initialized = true;
  snapshot_.listening = true;
  Serial.println("[Radio] READY: protocol=1 payload=12 channel=76 rate=250KBPS PA=LOW");
  return true;
}

void RadioManager::update(uint8_t alarmMask) {
  if (!snapshot_.initialized) return;
  pollReceive();
  const unsigned long now = millis();
  const unsigned long heartbeatInterval = snapshot_.consecutiveTxFailures >= 3
    ? RobotConfig::NRF_OFFLINE_HEARTBEAT_INTERVAL_MS
    : RobotConfig::NRF_HEARTBEAT_INTERVAL_MS;
  if (lastHeartbeatMs_ == 0 || now - lastHeartbeatMs_ >= heartbeatInterval) {
    lastHeartbeatMs_ = now;
    sendHeartbeat(alarmMask);
  }
}

void RadioManager::pollReceive() {
  uint8_t pipe = 0;
  uint8_t processed = 0;
  while (processed < 4 && radio_.available(&pipe)) {
    const uint8_t length = radio_.getDynamicPayloadSize();
    if (length != sizeof(RadioPacket)) {
      radio_.flush_rx();
      break;
    }
    RadioPacket packet;
    radio_.read(&packet, sizeof(packet));
    ++processed;
    if (!validatePacket(packet)) continue;
    ++snapshot_.rxPackets;
    snapshot_.lastRxMs = millis();
    snapshot_.lastPacketType = packet.packetType;
    snapshot_.lastSourceNode = packet.sourceNode;
    snapshot_.peerEverSeen = true;
    snapshot_.lastPeerSeenMs = snapshot_.lastRxMs;
  }
}

void RadioManager::sendHeartbeat(uint8_t alarmMask) {
  RadioPacket packet;
  packet.protocolVersion = RobotConfig::NRF_PROTOCOL_VERSION;
  packet.packetType = static_cast<uint8_t>(RadioPacketType::Heartbeat);
  packet.sourceNode = static_cast<uint8_t>(RadioNode::Piku21RevA);
  packet.sequence = ++sequence_;
  packet.alarmMask = alarmMask;
  packet.uptimeMs = millis();

  radio_.stopListening();
  snapshot_.listening = false;
  ++snapshot_.txAttempts;
  const bool sent = radio_.write(&packet, sizeof(packet));
  snapshot_.lastTxMs = millis();
  if (sent) {
    ++snapshot_.txSuccess;
    snapshot_.lastTxSuccessMs = snapshot_.lastTxMs;
    snapshot_.peerEverSeen = true;
    snapshot_.lastPeerSeenMs = snapshot_.lastTxMs;
    snapshot_.consecutiveTxFailures = 0;
  } else {
    ++snapshot_.txFailure;
    ++snapshot_.consecutiveTxFailures;
  }
  radio_.startListening();
  snapshot_.listening = true;
}

bool RadioManager::validatePacket(const RadioPacket& packet) const {
  if (packet.protocolVersion != RobotConfig::NRF_PROTOCOL_VERSION) return false;
  if (packet.sourceNode != static_cast<uint8_t>(RadioNode::NodeV1)) return false;
  return packet.packetType >= static_cast<uint8_t>(RadioPacketType::Heartbeat) &&
         packet.packetType <= static_cast<uint8_t>(RadioPacketType::Acknowledgement);
}

const RadioSnapshot& RadioManager::snapshot() const { return snapshot_; }
const char* RadioManager::hardwareStateName() const {
  if (!RobotConfig::ENABLE_NRF24) return "DISABLED";
  return snapshot_.initialized && snapshot_.chipDetected ? "READY" : "ERROR";
}
const char* RadioManager::peerStateName() const {
  if (!snapshot_.peerEverSeen) return "UNSEEN";
  return millis() - snapshot_.lastPeerSeenMs <= RobotConfig::NRF_PEER_STALE_MS ? "ONLINE" : "STALE";
}
const char* RadioManager::dataRateName() const { return "250KBPS"; }
const char* RadioManager::paLevelName() const { return "LOW"; }
