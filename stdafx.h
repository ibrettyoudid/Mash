// Copyright (c) 2023 Brett Curtis
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef _WIN32
#include "targetver.h"
#endif

#include <stdio.h>
#ifdef _WIN32
#include <tchar.h>
#endif


// TODO: reference additional headers your program requires here
#include <iostream>
#include <string>
#include <sstream>
#ifdef _MSC_VER
typedef unsigned  __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;

typedef signed  __int8 int8_t;
typedef signed __int16 int16_t;
typedef signed __int32 int32_t;
typedef signed __int64 int64_t;
#else
#include <stdint.h>
#endif
#include "stringops.h"
