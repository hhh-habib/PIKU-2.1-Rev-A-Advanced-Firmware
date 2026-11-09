#pragma once
#include "Arduino.h"
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <cstring>
using PGM_P=const char*;
constexpr int MSG_DONTWAIT=1, MSG_PEEK=2;
enum HTTPClientStatus { HC_NONE,HC_WAIT_READ,HC_WAIT_CLOSE };
constexpr size_t CONTENT_LENGTH_NOT_SET=0;
constexpr unsigned long HTTP_MAX_SEND_WAIT=1000,HTTP_MAX_CLOSE_WAIT=250;
struct SocketDouble { std::string input,output; bool connected=true; };
std::shared_ptr<SocketDouble> socketDouble,pendingSocket;
unsigned parseCalls=0;
enum class SendMode {Normal,Short,Stall,Reset};
SendMode sendMode=SendMode::Normal;
class Stream {public:void setTimeout(unsigned long){}};
class WiFiClient:public Stream {public:
 std::shared_ptr<SocketDouble> socket;
 WiFiClient()=default;explicit WiFiClient(std::shared_ptr<SocketDouble> s):socket(s){}
 explicit operator bool()const{return socket&&socket->connected;}
 bool connected()const{return static_cast<bool>(*this);}
 int available()const{return socket?static_cast<int>(socket->input.size()):0;}
 int fd()const{return connected()?7:-1;}
 void setTimeout(unsigned long){} void setNoDelay(bool){}
 void stop(){if(socket)socket->connected=false;}
 size_t write(const char* data,size_t n){
   if(sendMode==SendMode::Stall){testNow+=5000;return n;}
   socket->output.append(data,n);++testNow;return n;
 }
 size_t write_P(const char* b,size_t n){return write(b,n);}
};
class WebServer;
std::function<void(WebServer&)> handler;
class WebServer {public:
 explicit WebServer(int){} virtual void handleClient(){}
 void sendRam(const std::string& text){_currentClientWrite(text.data(),text.size());}
 void sendFlash(const std::string& text){_currentClientWrite_P(text.data(),text.size());}
 bool active()const{return _currentStatus!=HC_NONE;}
protected:
 struct Listener {WiFiClient available(){auto next=pendingSocket;pendingSocket.reset();return WiFiClient(next);}} _server;
 WiFiClient _currentClient;
 HTTPClientStatus _currentStatus=HC_NONE;
 bool _nullDelay=false;
 unsigned long _statusChange=0;
 size_t _contentLength=0;
 std::unique_ptr<int> _currentUpload,_currentRaw;
 virtual size_t _currentClientWrite(const char* b,size_t n){return _currentClient.write(b,n);}
 virtual size_t _currentClientWrite_P(PGM_P b,size_t n){return _currentClient.write_P(b,n);}
 bool _parseRequest(WiFiClient& client){
   ++parseCalls;const auto& text=client.socket->input;const auto end=text.find("\r\n\r\n");
   if(end==std::string::npos){testNow+=1000;return false;}
   const auto field=text.find("Content-Length:");
   const size_t body=field==std::string::npos?0:static_cast<size_t>(std::atoi(text.c_str()+field+15));
   if(text.size()<end+4+body){testNow+=1000;return false;}
   return true;
 }
 void _handleRequest(){if(handler)handler(*this);}
};
int testRecv(int,char* data,size_t n,int){
 if(!socketDouble||!socketDouble->connected)return 0;
 const auto count=std::min(n,socketDouble->input.size());
 std::memcpy(data,socketDouble->input.data(),count);return static_cast<int>(count);
}
int testSend(int,const char* data,size_t n,int){
 if(sendMode==SendMode::Stall){errno=EAGAIN;return -1;}
 if(sendMode==SendMode::Reset){errno=ECONNRESET;return -1;}
 const auto count=sendMode==SendMode::Short?std::min(n,size_t(512)):n;
 socketDouble->output.append(data,count);++testNow;return static_cast<int>(count);
}
