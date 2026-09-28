#ifndef MANUAL_MOTION_GATE_H
#define MANUAL_MOTION_GATE_H

enum class ManualDirection { Forward, Backward, Left, Right, Release, Stop };
enum class ManualInhibit {
  None, Release, OperatorStop, Mode, StaleCommand, SessionLost,
  TemperatureCritical, SensorFault, FlameGas, ForwardObstacle,
  PivotClearanceUnknown
};
struct ManualGateInput {
  ManualDirection direction = ManualDirection::Stop;
  bool manualMode = false;
  bool staleCommand = false;
  bool flameGasCritical = false;
  bool temperatureCritical = false;
  bool sensorFault = false;
  bool overrideEnabled = false;
  bool sessionAlive = false;
  bool forwardObstacle = false;
  bool pivotSweepVerified = false;
};
inline ManualInhibit manualMotionGate(const ManualGateInput& in) {
  if (in.direction == ManualDirection::Stop) return ManualInhibit::OperatorStop;
  if (in.direction == ManualDirection::Release) return ManualInhibit::Release;
  if (!in.manualMode) return ManualInhibit::Mode;
  if (in.staleCommand) return ManualInhibit::StaleCommand;
  if (in.overrideEnabled && !in.sessionAlive) return ManualInhibit::SessionLost;
  if (in.temperatureCritical) return ManualInhibit::TemperatureCritical;
  if (in.sensorFault) return ManualInhibit::SensorFault;
  if (in.flameGasCritical && !in.overrideEnabled) return ManualInhibit::FlameGas;
  if (in.direction == ManualDirection::Forward && in.forwardObstacle)
    return ManualInhibit::ForwardObstacle;
  if ((in.direction == ManualDirection::Left || in.direction == ManualDirection::Right) &&
      !in.pivotSweepVerified)
    return ManualInhibit::PivotClearanceUnknown;
  // Backward requires a fresh held operator command; no rear sensor is implied.
  return ManualInhibit::None;
}
inline const char* manualInhibitName(ManualInhibit reason) {
  switch (reason) {
    case ManualInhibit::None: return "NONE";
    case ManualInhibit::Release: return "MANUAL_RELEASE";
    case ManualInhibit::OperatorStop: return "OPERATOR_STOP";
    case ManualInhibit::Mode: return "MODE";
    case ManualInhibit::StaleCommand: return "MANUAL_DEADMAN";
    case ManualInhibit::SessionLost: return "CONTROL_SESSION_LOST";
    case ManualInhibit::TemperatureCritical: return "TEMPERATURE_CRITICAL";
    case ManualInhibit::SensorFault: return "SENSOR_FAULT";
    case ManualInhibit::FlameGas: return "FLAME_GAS";
    case ManualInhibit::ForwardObstacle: return "FORWARD_OBSTACLE";
    case ManualInhibit::PivotClearanceUnknown: return "PIVOT_CLEARANCE_UNKNOWN";
  }
  return "UNKNOWN";
}
#endif
