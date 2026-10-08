#include <cassert>
#include "ManualMotionGate.h"
int main() {
  ManualGateInput x;
  x.manualMode = true;
  x.direction = ManualDirection::Forward;
  assert(manualMotionGate(x) == ManualInhibit::None);
  x.manualMode = false;
  assert(manualMotionGate(x) == ManualInhibit::Mode);
  x.manualMode = true;
  x.flameGasCritical = true;
  assert(manualMotionGate(x) == ManualInhibit::FlameGas);
  x.overrideEnabled = true; x.sessionAlive = true;
  assert(manualMotionGate(x) == ManualInhibit::None);
  x.temperatureCritical = true;
  assert(manualMotionGate(x) == ManualInhibit::TemperatureCritical);
  x.temperatureCritical = false; x.sensorFault = true;
  assert(manualMotionGate(x) == ManualInhibit::SensorFault); // flame sample fault only
  x.sensorFault = false; x.forwardObstacle = true;
  assert(manualMotionGate(x) == ManualInhibit::ForwardObstacle);
  x.forwardObstacle = false; x.direction = ManualDirection::Left;
  assert(manualMotionGate(x) == ManualInhibit::None); // supervised turn
  x.leftTurnObstacle = true;
  assert(manualMotionGate(x) == ManualInhibit::TurnObstacle);
  x.leftTurnObstacle = false; x.direction = ManualDirection::Right;
  assert(manualMotionGate(x) == ManualInhibit::None);
  x.rightTurnObstacle = true;
  assert(manualMotionGate(x) == ManualInhibit::TurnObstacle);
  x.rightTurnObstacle = false; x.direction = ManualDirection::Backward;
  assert(manualMotionGate(x) == ManualInhibit::None); // supervised rear
  x.staleCommand = true;
  assert(manualMotionGate(x) == ManualInhibit::StaleCommand);
  x.staleCommand = false; x.sessionAlive = false;
  assert(manualMotionGate(x) == ManualInhibit::SessionLost);
  x.direction = ManualDirection::Release;
  assert(manualMotionGate(x) == ManualInhibit::Release);
  ManualGateInput supervised;
  supervised.manualMode = true;
  supervised.direction = ManualDirection::Forward;
  supervised.flameGasCritical = true;
  supervised.overrideEnabled = true;
  supervised.sessionAlive = true;
  assert(manualMotionGate(supervised) == ManualInhibit::None);
  supervised.forwardObstacle = true; // known sonar or front IR
  assert(manualMotionGate(supervised) == ManualInhibit::ForwardObstacle);
  supervised.forwardObstacle = false;
  supervised.direction = ManualDirection::Backward;
  assert(manualMotionGate(supervised) == ManualInhibit::None);
  supervised.direction = ManualDirection::Left;
  assert(manualMotionGate(supervised) == ManualInhibit::None);
  supervised.direction = ManualDirection::Right;
  assert(manualMotionGate(supervised) == ManualInhibit::None);
  supervised.staleCommand = true;
  assert(manualMotionGate(supervised) == ManualInhibit::StaleCommand);
  supervised.staleCommand = false;
  supervised.sessionAlive = false;
  assert(manualMotionGate(supervised) == ManualInhibit::SessionLost);
  supervised.sessionAlive = true;
  supervised.temperatureCritical = true;
  assert(manualMotionGate(supervised) == ManualInhibit::TemperatureCritical);
  const float stopCm = 20.0f, clearCm = 25.0f;
  bool sonarBlocked = false;
  sonarBlocked = manualForwardSonarBlocked(
      sonarBlocked, ManualSonarEvidence::DistanceValid, 19.0f, stopCm, clearCm);
  assert(sonarBlocked);
  sonarBlocked = manualForwardSonarBlocked(
      false, ManualSonarEvidence::DistanceValid, 20.0f, stopCm, clearCm);
  assert(sonarBlocked);
  assert(manualForwardSonarBlocked(
      true, ManualSonarEvidence::DistanceValid, 23.0f, stopCm, clearCm));
  assert(!manualForwardSonarBlocked(
      false, ManualSonarEvidence::DistanceValid, 23.0f, stopCm, clearCm));
  sonarBlocked = manualForwardSonarBlocked(
      true, ManualSonarEvidence::DistanceValid, 25.0f, stopCm, clearCm);
  assert(!sonarBlocked);
  assert(!manualForwardSonarBlocked(
      true, ManualSonarEvidence::ClearNoEcho, -1.0f, stopCm, clearCm));
  assert(!manualForwardSonarBlocked(
      true, ManualSonarEvidence::Unknown, -1.0f, stopCm, clearCm));
  x.direction = ManualDirection::Stop;
  assert(manualMotionGate(x) == ManualInhibit::OperatorStop);
}
