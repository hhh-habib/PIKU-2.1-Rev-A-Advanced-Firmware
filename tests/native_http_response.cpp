#include <cassert>
int main(int argc,char** argv){
 assert(argc==2);const std::string name=argv[1];PriorityWebServer server;
 socketDouble=std::make_shared<SocketDouble>();pendingSocket=socketDouble;
 const std::string get="GET /ping HTTP/1.1\r\nHost: robot\r\n\r\n";
 socketDouble->input=get;unsigned handled=0;
 handler=[&](WebServer&){++handled;};
 if(name=="fragmented_header"){
   socketDouble->input="GET /ping HTTP/1.1\r\nHost:";const auto start=testNow;server.handleClient();
   assert(testNow-start<5&&parseCalls==0&&server.active());
   testNow+=20;socketDouble->input=get;server.handleClient();assert(handled==1&&parseCalls==1);
 }else if(name=="fragmented_body"){
   socketDouble->input="POST /speed HTTP/1.1\r\nHost: robot\r\nContent-Length: 5\r\n\r\n";
   const auto start=testNow;server.handleClient();assert(testNow-start<5&&parseCalls==0);
   testNow+=20;socketDouble->input+="set=5";server.handleClient();assert(handled==1);
 }else if(name=="absolute_request_deadline"){
   socketDouble->input="GET /ping HTTP/1.1\r\nHost:";server.handleClient();testNow+=250;
   server.handleClient();assert(!server.active()&&server.timeoutCount()==1&&parseCalls==0);
 }else if(name=="oversized_body"||name=="duplicate_length"||name=="chunked"){
   socketDouble->input="POST /speed HTTP/1.1\r\nHost: robot\r\n";
   socketDouble->input+=name=="oversized_body"?"Content-Length: 999999999999999999999\r\n":
     name=="duplicate_length"?"Content-Length: 0\r\nContent-Length: 0\r\n":"Transfer-Encoding: chunked\r\n";
   socketDouble->input+="\r\n";server.handleClient();assert(parseCalls==0);testNow+=250;
   server.handleClient();assert(!server.active()&&server.timeoutCount()==1);
 }else if(name=="stalled_control"||name=="stalled_flash"||name=="stalled_telemetry"){
   sendMode=name=="stalled_flash"?SendMode::Normal:SendMode::Stall;
   handler=[&](WebServer& web){
     if(name=="stalled_telemetry")server.beginTelemetryResponse();
     web.sendRam("HTTP/1.1 200 OK\r\n\r\n");
     if(name=="stalled_flash"){sendMode=SendMode::Stall;web.sendFlash(std::string(67000,'x'));}else web.sendRam("OK");
   };
   const auto start=testNow;server.handleClient();
   const auto limit=name=="stalled_control"?25UL:name=="stalled_flash"?250UL:125UL;
   assert(testNow-start<=limit+1&&server.timeoutCount()==1&&!socketDouble->connected);
 }else if(name=="flash_short_writes"){
   sendMode=SendMode::Short;const std::string body(67000,'x');
   handler=[&](WebServer& web){web.sendRam("HEADER");web.sendFlash(body);};
   server.handleClient();assert(socketDouble->output=="HEADER"+body&&server.timeoutCount()==0);
 }else if(name=="reset_does_not_spin"){
   sendMode=SendMode::Reset;handler=[](WebServer& web){web.sendRam("OK");web.sendRam("again");};
   const auto start=testNow;server.handleClient();assert(testNow-start<5&&server.timeoutCount()==1);
 }else if(name=="complete_get"){
   server.handleClient();assert(handled==1&&parseCalls==1&&server.timeoutCount()==0);
 }else return 2;
}
