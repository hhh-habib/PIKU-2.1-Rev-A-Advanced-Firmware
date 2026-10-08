// Owner-approved V3.2 expectations; actual NavigationManager and existing host runner.
#define V32_SCENARIOS_LIBRARY
#include "native_v32_restoration.cpp"
int main(int argc,char** argv){assert(argc==2);
 if(!std::strcmp(argv[1],"single")){restoredScenario("clear_single");return 0;}
 if(!std::strcmp(argv[1],"boxed")){restoredScenario("side6");return 0;}
 if(!std::strcmp(argv[1],"resume")){restoredScenario("ir_turn");return 0;}
 if(!std::strcmp(argv[1],"clear")){restoredScenario("exit");return 0;}
 if(!std::strcmp(argv[1],"sixth")){restoredScenario("exhaustion");return 0;}
 if(!std::strcmp(argv[1],"both")){restoredScenario("both");return 0;}
 if(!std::strcmp(argv[1],"accepted")){restoredScenario("front");return 0;}
 if(!std::strcmp(argv[1],"both_stop")){restoredScenario("stop_reverse");return 0;}
 if(!std::strcmp(argv[1],"both_transfer")){restoredScenario("pulse_limit");return 0;}
 if(!std::strcmp(argv[1],"both_holds")){restoredScenario("environment_sonar");return 0;}
 if(!std::strcmp(argv[1],"retired_work")){restoredScenario("stop_recovery");return 0;}
 if(!std::strcmp(argv[1],"normal_checkpoint")){restoredScenario("left");return 0;}
 if(!std::strcmp(argv[1],"scan_validity")){restoredScenario("history");return 0;}
 if(!std::strcmp(argv[1],"chosen_side")){restoredScenario("side7");return 0;}
 if(!std::strcmp(argv[1],"handoff")){restoredScenario("exit");return 0;}
 if(!std::strcmp(argv[1],"direction")){restoredScenario("right");return 0;}
 if(!std::strcmp(argv[1],"boundary")){restoredScenario("timeout");return 0;}
 if(!std::strcmp(argv[1],"safety_races")){restoredScenario("critical");return 0;}
 if(!std::strcmp(argv[1],"boot")){restoredScenario("normal");return 0;}
 if(!std::strcmp(argv[1],"environment")){restoredScenario("hazard");return 0;}
 if(!std::strcmp(argv[1],"sixth_resume")){restoredScenario("ir_turn");return 0;}
 if(!std::strcmp(argv[1],"clock_pause")){restoredScenario("timeout");return 0;}
 if(!std::strcmp(argv[1],"saved_phases")){restoredScenario("front_after");return 0;}
 if(!std::strcmp(argv[1],"incidental")){restoredScenario("open_exit");return 0;}
 if(!std::strcmp(argv[1],"ten_pulses")){restoredScenario("retry");return 0;}
 assert(!"unknown legacy navigation scenario");
}
