#pragma once
#include <Arduino.h>
#include <array>
#include <vector>
struct WireDouble {
  bool present=true, fail=false, shortRead=false, dataReady=true, zeros=false, ones=false;
  bool badConfig=false, moving=false;
  bool resetStuck=false;
  uint8_t device=0x68, identity=0x68, address=0, reg=0;
  uint16_t timeout=50;
  unsigned writes=0, writesMoving=0, reads=0;
  std::array<int16_t,7> raw{{0,0,8192,0,0,0,0}};
  std::array<uint8_t, 128> registers{};
  std::vector<uint8_t> tx, rx;
  size_t offset=0;
  uint16_t getTimeOut() const { return timeout; }
  void setTimeOut(uint16_t n) { timeout=n; }
  void beginTransmission(uint8_t a) { address=a; tx.clear(); }
  size_t write(uint8_t b) { tx.push_back(b); return 1; }
  uint8_t endTransmission(bool=true) {
    if (!present || fail || address!=device) return 4;
    reg=tx.at(0);
    if (tx.size()>1) {
      ++writes; if(moving) ++writesMoving;
      registers.at(reg)=tx[1];
      if(reg==0x6b && tx[1]==0x80 && !resetStuck) registers.fill(0);
    }
    return 0;
  }
  size_t requestFrom(uint8_t, size_t n, bool) {
    ++reads; rx.clear(); offset=0;
    if (!present || fail) return 0;
    for(size_t i=0;i<n;++i) {
      uint8_t r=reg+static_cast<uint8_t>(i), v=registers.at(r);
      if(r==0x75) v=identity;
      if(r==0x3a) v=dataReady?1:0;
      if(r>=0x3b && r<=0x48) {
        const unsigned k=r-0x3b;
        v=uint8_t(uint16_t(raw[k/2]) >> (k%2?0:8));
        if(zeros) v=0;
        if(ones) v=0xff;
      }
      if(badConfig && r==0x1b) v=0;
      rx.push_back(v);
    }
    if(shortRead && !rx.empty()) rx.pop_back();
    return rx.size();
  }
  int available() const { return static_cast<int>(rx.size()-offset); }
  int read() { return offset<rx.size()?rx[offset++]:-1; }
};
extern WireDouble Wire;
