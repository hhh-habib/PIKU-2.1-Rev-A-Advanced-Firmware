#pragma once

// Arduino WebServer 2.0.0 hard-codes five-second ownership windows. Force the
// header to load first, then narrow those macros for every C++ project/library
// TU. C translation units must not include the C++ WebServer interface.
#ifdef __cplusplus
#include <WebServer.h>

#undef HTTP_MAX_DATA_WAIT
#undef HTTP_MAX_POST_WAIT
#undef HTTP_MAX_SEND_WAIT
#undef HTTP_MAX_CLOSE_WAIT

#define HTTP_MAX_DATA_WAIT 250
#define HTTP_MAX_POST_WAIT 1000
#define HTTP_MAX_SEND_WAIT 1000
#define HTTP_MAX_CLOSE_WAIT 250
#endif
