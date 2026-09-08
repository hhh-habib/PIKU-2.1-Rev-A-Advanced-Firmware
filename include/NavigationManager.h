#ifndef NAVIGATIONMANAGER_H
#define NAVIGATIONMANAGER_H

#include <Arduino.h>
#include "MotorController.h"
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
  MotorController& _motors;
  SensorManager& _sensors;
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
  float scanAt(int angle, const String& name);
  float readAndPublishDistance(const String& servoPos);
  bool acquireFreshFrontDistance(float& distanceCm);
  bool driveForwardWithFreshGuard();
  void runObstacleAvoidance();
  void choosePathAndTurn(float left, float center, float right, bool nearRecovery);
  bool runNearObstacleRecovery();
  bool abortIfOperatorRequested();
  bool abortIfCritical();
};

#endif // NAVIGATIONMANAGER_H
