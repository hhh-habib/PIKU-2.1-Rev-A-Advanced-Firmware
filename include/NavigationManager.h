#ifndef NAVIGATIONMANAGER_H
#define NAVIGATIONMANAGER_H

#include <Arduino.h>
#include "ImuManager.h"
#include "MotorController.h"
#include "RobotConfig.h"
#include "SafetyManager.h"
#include "SensorManager.h"
#include "ServoScanner.h"
#include "WebDashboard.h"

enum class NavigationState {
  Idle,
  ScanFront,
  Forward,
  ObstacleReverse,
  ScanLeft,
  ScanRight,
  TurnLeft,
  TurnRight,
  InvalidStop,
  NearObstacleHold,
  NearObstacleRecovery,
  NearObstacleReverse,
  NearObstacleScan,
  NearObstacleTurn,
  NearObstacleCooldown,
  OperatorAbort,
  CriticalAbort
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
  NavigationState state() const;
  const char* stateName() const;

private:
  struct NavigationScanSample {
    int angle = RobotConfig::CENTER_SCAN;
    const char* direction = "UNKNOWN";
    UltrasonicSample ultrasonic;
    bool completed = false;
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
  unsigned long _lastNearRecoveryMs;
  bool _nearRecoveryLatched;
  bool _operatorStopLatched;
  unsigned long _operatorStopModeCommandCounter;

  void setState(NavigationState state, const String& decision);
  bool responsiveDelay(unsigned long durationMs);
  NavigationScanSample scanAt(int angle, const char* name);
  UltrasonicSample readAndPublishDistance(const char* servoPos);
  bool acquireFreshFrontDistance(UltrasonicSample& sample);
  bool driveForwardWithFreshGuard();
  bool executeAvoidanceTurn(bool turnLeft, bool nearRecovery);
  void runObstacleAvoidance();
  void choosePathAndTurn(const NavigationScanSample& left,
                         const NavigationScanSample& center,
                         const NavigationScanSample& right,
                         bool nearRecovery);
  bool scanSampleTrusted(const NavigationScanSample& sample) const;
  float scanPathDistance(const NavigationScanSample& sample) const;
  bool runNearObstacleRecovery();
  bool abortIfOperatorRequested();
  bool abortIfCritical();
};

#endif // NAVIGATIONMANAGER_H
