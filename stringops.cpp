#include "stdafx.h"
#include "stringops.h"
#include <sstream>
#include <fstream>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif

std::string TS;

int64_t debugLevel = 0;
int64_t debugFlag = true;

namespace std
{

   string operator+(const string& lhs, int64_t rhs)
   {
      stringstream ss;
      ss << lhs;
      ss << rhs;
      return ss.str();
   }

   string& operator+=(string& lhs, int64_t rhs)
   {
      stringstream ss;
      ss << lhs;
      ss << rhs;
      lhs = ss.str();
      return lhs;
   }

   struct file_not_found{};

   string loadString(string name)
   {
   #ifdef _WINDOWS
      std::ifstream strm(name.c_str(), std::ios_base::in | std::ios_base::binary, _SH_DENYNO);
   #else
      std::ifstream strm(name.c_str(), std::ios_base::in | std::ios_base::binary);
   #endif
      if (strm.fail())
         throw file_not_found();
      strm.seekg(0, ios_base::end);
      string result(strm.tellg(), ' ');
      strm.seekg(0);
      strm.read(&result[0], result.size());
      return result;
   }

   void saveString(const string& str, string name)
   {
      std::ofstream strm(name.c_str(), std::ios_base::out | std::ios_base::binary);
      strm.write(&str[0], str.size());
   }

   string pad(string In, size_t Len)
   {
      if (In.size() < Len)
         return In + string(Len - In.size(), ' ');
      else if (In.size() > Len)
         return In.substr(0, Len);
      else
         return In;
   }

   string lpad(string In, size_t Len)
   {
      if (In.size() < Len)
         return string(Len - In.size(), ' ') + In;
      else if (In.size() > Len)
         return In.substr(0,  Len);
      else
         return In;
   }
   void ODST(string s)
   {
      if (debugFlag)
      #ifdef _WIN32
         OutputDebugStringA((s+"\n").data());
      #else
   	   cout << s << endl;
      #endif
   }

   void ODSL(string S)
   {
      ODST(string(debugLevel * 3, ' ')+S);
   }
   void ODSI(string S)
   {
      ODST(string(debugLevel * 3, ' ')+S);
      ++debugLevel;
   }
   void ODSO(string S)
   {
      --debugLevel;
      ODST(string(debugLevel * 3, ' ')+S);
   }


};