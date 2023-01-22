#include "stdafx.h"
#include "stringops.h"
#include <sstream>
#include <fstream>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif

std::string TS;

int debugLevel = 0;
int debugFlag = true;

namespace std
{

   string operator+(const string& lhs, int rhs)
   {
      stringstream ss;
      ss << lhs;
      ss << rhs;
      return ss.str();
   }

   string& operator+=(string& lhs, int rhs)
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
      std::ifstream strm(name.c_str(), std::ios_base::in, _SH_DENYNO);
   #else
      std::ifstream strm(name.c_str(), std::ios_base::in);
   #endif
      if (strm.fail())
         throw file_not_found();
      strm.seekg(0, ios_base::end);
      string result(strm.tellg(), ' ');
      strm.seekg(0);
      strm.read(const_cast<char*>(result.c_str()), result.size());
      return result;
   }

   void saveString(const string& str, string name)
   {
      std::ofstream strm(name.c_str(), std::ios_base::out);
      strm.write(str.c_str(), str.size());
   }

   string pad(string In, int Len)
   {
      if ((int)In.size() < Len)
         return In + string(Len - In.size(), ' ');
      else if ((int)In.size() > Len)
         return In.substr(0, Len);
      else
         return In;
   }

   string lpad(string In, int Len)
   {
      if ((int)In.size() < Len)
         return string(Len - In.size(), ' ') + In;
      else if ((int)In.size() > Len)
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