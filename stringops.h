// Copyright (c) 2023 Brett Curtis
#include <string>
#include <sstream>

#pragma once

extern std::string TS;
extern int64_t debugLevel;
extern int64_t debugFlag;
namespace std
{
#ifndef _UNICODE
typedef  string       string;
typedef  stringstream tstringstream;
typedef  fstream      tfstream;
typedef  ifstream     tifstream;
typedef  ofstream     tofstream;
#else
typedef wstring       string;
typedef wstringstream tstringstream;
typedef wfstream      tfstream;
typedef wifstream     tifstream;
typedef wofstream     tofstream;
#endif
template <class T>
T fromString(std::string s)
{
   std::stringstream ss(s);
   T n = 0;
   ss >> n;
   return n;
}

template <class T>
std::string toString(T n)
{
   std::stringstream ss;
   ss << n;
   return ss.str();
}

std::string  operator+ (const std::string& lhs, int64_t rhs);
std::string& operator+=(std::string& lhs, int64_t rhs);
std::string  loadString(std::string  name);
void         saveString(const std::string& str, std::string name);
std::string  pad       (std::string  str, size_t len);
std::string  lpad      (std::string  str, size_t len);

void ODST(std::string s);
void ODSL(std::string s);
void ODSI(std::string s);
void ODSO(std::string s);
}