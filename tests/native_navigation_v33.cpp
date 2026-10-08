// Retains the old production runner, using the approved per-mission behavior.
#define V32_SCENARIOS_LIBRARY
#include "native_v32_restoration.cpp"
int main(){for(const char* scenario:{"ir_turn","front","exit","invalid","stale","open","hazard","critical","stop_recovery"})restoredScenario(scenario);}
