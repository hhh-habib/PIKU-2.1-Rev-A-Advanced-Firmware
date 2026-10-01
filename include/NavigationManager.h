#ifndef NAVIGATIONMANAGER_H
#define NAVIGATIONMANAGER_H

#include <Arduino.h>
#include "ImuManager.h"
#include "MotorController.h"
#include "RobotConfig.h"
#include "RecoveryIrBackoffPolicy.h"
#include "SafetyManager.h"
#include "SensorManager.h"
#include "ServoScanner.h"
#include "StuckRecoveryV2Policy.h"
#include "V2EscapeDecision.h"
#include "WebDashboard.h"

enum class NavigationState {
  Idle,
  FrontSettle,
  FrontSample,
  Forward,
  ObstacleReverse,
  ScanLeft,
  ScanFront,
  ScanRight,
  Recenter,
  SpinLeft,
  SpinRight,
  InvalidStop,
  ObstacleRecovery,
  ObstacleScan,
  ObstacleSpin,
  ObstacleReevaluate,
  BlockedRetryWait,
  HazardActive,
  HazardClearWait,
  HazardRecoveryScan,
  StuckSuspected,
  StuckRecovery,
  RecoveryIrBackoff,
  OperatorAbort,
  CriticalAbort,
  TrappedHold
};

class NavigationManager {
public:
  using ServiceCallback = bool (*)();
  using PublishCallback = void (*)(bool force);
  using WarningCallback = void (*)();

  NavigationManager(MotorController& motors,
                    SensorManager& sensors,
                    ImuManager& imu,
                    ServoScanner& servoScanner,
                    SafetyManager& safety,
                    WebDashboard& dashboard,
                    DashboardData& dashboardData,
                    String& currentServoPosition,
                    ServiceCallback serviceCallback,
                    PublishCallback publishCallback,
                    WarningCallback warningCallback);

  void updateAuto();
  void resetRecoveryLatchIfClear();
  void notifyOperatorStop();
  void cancelForModeChange();
  bool environmentalRecoveryInProgress() const;
  bool consumeEnvironmentalRecoveryCompleted();
  NavigationState state() const;
  const char* stateName() const;

private:
  enum class AutoPhase {
    Idle,
    FrontSettle,
    FrontSampleWait,
    Forward,
    ForwardSettle,
    ReversePrepare,
    Reverse,
    ReverseSettle,
    ScanServoSettle,
    ScanSampleGap,
    ScanSampleWait,
    RecenterSettle,
    Turn,
    PostTurnSettle,
    HazardActive,
    HazardClearWait,
    InvalidStop,
    OperatorStop,
    BlockedRetryWait,
    RecoveryFrontSettle,
    RecoveryFrontWait,
    RecoveryIrBackoff,
    TrappedHold
  };

  enum class ScanPurpose {
    Obstacle,
    IrSingle,
    IrBoth,
    EnvironmentalRecovery,
    StuckRecovery,
    EscapeV2,
    RecoverySide
  };

  enum class IrZone {
    Clear,
    Left,
    Right,
    Both
  };

  struct NavigationScanSample {
    int angle = RobotConfig::CENTER_SCAN;
    const char* direction = "UNKNOWN";
    UltrasonicSample ultrasonic;
    bool completed = false;
    uint8_t trustedReadings = 0;
    uint8_t finiteReadings = 0;
    float spreadCm = NAN;
  };

  struct ScanAccumulator {
    float distances[RobotConfig::AUTO_SCAN_SAMPLES_PER_DIRECTION]{};
    uint8_t distanceCount = 0;
    uint8_t noEchoCount = 0;
    uint8_t trustedCount = 0;
    uint8_t sensorFaultCount = 0;
    unsigned long newestSampleMs = 0;
  };

  MotorController& _motors;
  SensorManager& _sensors;
  ImuManager& _imu;
  ServoScanner& _servoScanner;
  SafetyManager& _safety;
  WebDashboard& _dashboard;
  DashboardData& _dashboardData;
  String& _currentServoPosition;
  ServiceCallback _serviceCallback;
  PublishCallback _publishCallback;
  WarningCallback _warningCallback;

  NavigationState _state;
  AutoPhase _phase;
  ScanPurpose _scanPurpose;
  unsigned long _phaseStartedMs;
  unsigned long _lastUpdateMs;
  unsigned long _sampleTargetSequence;
  unsigned long _sampleRequestedMs;
  unsigned long _lastGuardSequence;
  uint8_t _scanDirectionIndex;
  uint8_t _scanReadingIndex;
  ScanAccumulator _scanAccumulator;
  NavigationScanSample _leftScan;
  NavigationScanSample _frontScan;
  NavigationScanSample _rightScan;

  bool _turnLeft;
  bool _turnUsingGyro;
  float _turnGyroBiasRadPerSecond;
  float _relativeTurnDeg;
  unsigned long _turnStartedMs;
  unsigned long _lastIntegratedSampleMs;
  bool _lastAvoidanceTurnLeft;

  IrZone _irEscapeSourceZone;
  bool _obstacleRecoveryActive;
  bool _obstacleReevaluationPending;
  uint8_t _obstacleRecoveryAttempts;
  uint16_t _blockedRetryCount;
  unsigned long _obstacleForwardClearMs;
  bool _operatorStopLatched;
  unsigned long _operatorStopModeCommandCounter;

  bool _environmentalRecoveryActive;
  bool _environmentalRecoveryCompleted;
  unsigned long _hazardClearStartedMs;

  float _stuckDistanceSamples[RobotConfig::AUTO_STUCK_DISTANCE_SAMPLE_COUNT];
  uint8_t _stuckDistanceCount;
  uint8_t _stuckDistanceIndex;
  unsigned long _stuckForwardActiveMs;
  unsigned long _stuckImuStillMs;
  unsigned long _stuckRecoveryForwardMs;
  uint8_t _stuckRecoveryAttempts;
  bool _stuckRecoveryTrialActive;
  String _lastStuckReason;
  StuckRecoveryV2Policy _v2;
  String _v2LastReason = "NONE";
  unsigned long _v2PriorForwardMs = 0;
  uint8_t _recoverySideRechecks = 0;
  RecoveryIrBackoffPolicy _recoveryIrBackoff;

  void setState(NavigationState state, const String& decision);
  void setPhase(AutoPhase phase, unsigned long now);
  bool handleOperatorInterruption();
  bool handleCriticalSafety(unsigned long now);
  void beginFrontValidation(unsigned long now, const char* decision);
  void updateFrontValidation(unsigned long now);
  void startForward(unsigned long now, const char* decision);
  void updateForward(unsigned long now, unsigned long elapsedMs);
  void beginObstacleRecovery(unsigned long now);
  bool handleIrArbitration(unsigned long now);
  void beginIrRecovery(IrZone zone, unsigned long now);
  void beginUnifiedObstacleRecovery(ScanPurpose purpose, IrZone irSource, unsigned long now);
  void enterBlockedRetryWait(const char* reason, unsigned long now);
  void updateBlockedRetryWait(unsigned long now);
  IrZone currentIrZone() const;
  const char* irZoneName(IrZone zone) const;
  void updateReverseSequence(unsigned long now);
  void beginScan(ScanPurpose purpose, unsigned long now);
  void commandCurrentScanDirection(unsigned long now);
  void updateScan(unsigned long now);
  void recordScanReading(const UltrasonicSample& sample);
  NavigationScanSample finalizeCurrentScan() const;
  void storeCurrentScan(const NavigationScanSample& sample);
  void beginRecenter(unsigned long now);
  void selectPathAfterScan(unsigned long now);
  bool selectClearestSide(bool& turnLeft) const;
  void beginSpinTurn(bool turnLeft, ScanPurpose purpose, unsigned long now);
  void updateSpinTurn(unsigned long now);
  void completeSpin(const char* reason, unsigned long now);
  void finishRecoveryAfterSettle(unsigned long now);
  void enterInvalidStop(const char* reason);
  void enterEnvironmentalHazard(unsigned long now);
  void updateEnvironmentalRecovery(unsigned long now);
  void completeEnvironmentalRecovery();
  void resetStuckEvidence(bool clearAttempts);
  void updateStuckEvidence(unsigned long now, unsigned long elapsedMs);
  void recordStuckDistance(const UltrasonicSample& sample);
  bool stuckDistanceUnchanged() const;
  bool stuckEvidenceSatisfied() const;
  void beginStuckRecovery(unsigned long now);
  void beginV2Escape(unsigned long now, bool reuseScan);
  void selectRecoveryDirection(unsigned long now);
  void beginRecoverySideCheck(unsigned long now);
  void finishRecoverySideCheck(unsigned long now);
  void beginRecoveryFrontCheck(unsigned long now);
  void updateRecoveryFrontCheck(unsigned long now);
  void beginRecoveryIrBackoff(unsigned long now, bool requireFirstPulse=false);
  void updateRecoveryIrBackoff(unsigned long now);
  void updateV2RetryCooldown(unsigned long now);
  void beginV2Turn(bool turnLeft, unsigned long now);
  bool v2ImuReady(unsigned long now) const;
  void enterV2TrappedHold(const char* reason, unsigned long now);
  bool scanSampleTrusted(const NavigationScanSample& sample) const;
  RecoveryReading recoveryReading(const NavigationScanSample& sample) const;
  float scanPathClearance(const NavigationScanSample& sample) const;
  const char* ultrasonicStateName(UltrasonicResultState state) const;
  const char* phaseName() const;
  void updateDiagnostics(unsigned long now);
};

#endif // NAVIGATIONMANAGER_H
