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
  assert(manualMotionGate(x) == ManualInhibit::SensorFault);
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
  x.direction = ManualDirection::Stop;
  assert(manualMotionGate(x) == ManualInhibit::OperatorStop);
}
