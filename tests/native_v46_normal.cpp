// Owner-approved V3.2 expectations; actual NavigationManager and existing host runner.
#define V32_SCENARIOS_LIBRARY
#include "native_v32_restoration.cpp"
int main(int argc,char** argv){assert(argc==2);
 if(!std::strcmp(argv[1],"single")){restoredScenario("clear_single");return 0;}
 if(!std::strcmp(argv[1],"post_blocked")){restoredScenario("left");return 0;}
 if(!std::strcmp(argv[1],"front")){restoredScenario("front");return 0;}
 if(!std::strcmp(argv[1],"both")){restoredScenario("both");return 0;}
 if(!std::strcmp(argv[1],"opposite")){restoredScenario("right");return 0;}
 if(!std::strcmp(argv[1],"boxed")){restoredScenario("boxed");return 0;}
 if(!std::strcmp(argv[1],"no_path")){restoredScenario("no_path");return 0;}
 if(!std::strcmp(argv[1],"entry")){restoredScenario("mode");return 0;}
 if(!std::strcmp(argv[1],"long")){restoredScenario("history");return 0;}
 if(!std::strcmp(argv[1],"races")){restoredScenario("stop_turn");return 0;}
 if(!std::strcmp(argv[1],"normal_boxed_lock")){restoredScenario("ir_turn");return 0;}
 if(!std::strcmp(argv[1],"imu_bounds")){restoredScenario("normal_fallback");restoredScenario("normal_deadline");return 0;}
 assert(!"unknown legacy navigation scenario");
}
