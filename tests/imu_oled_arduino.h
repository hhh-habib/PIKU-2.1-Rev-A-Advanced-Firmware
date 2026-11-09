#pragma once
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <string>
using String = std::string;
using std::isfinite;
#define HIGH 1
#define LOW 0
#define RAD_TO_DEG 57.29577951308232f
#define DEG_TO_RAD 0.017453292519943295f
extern unsigned long testNow;
inline unsigned long millis() { return testNow; }
extern unsigned delayCalls;
inline void delay(unsigned long) { ++delayCalls; }
struct SerialDouble { template<class T> void println(const T&) {} };
extern SerialDouble Serial;
