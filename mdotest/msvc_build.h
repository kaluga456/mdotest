#pragma once

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

//common headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//platform headers
#include "winsock2.h"
#pragma comment(lib, "Ws2_32.lib")
#define socklen_t int
#define ssize_t SSIZE_T
#define close closesocket
