// Copyright (c) 2023 Brett Curtis
#include "stdafx.h"
#include "TimerFrame.h"
#include <vector>
#include <math.h>
#include <stdio.h>

#include "stringops.h"

#ifdef __linux__
#include <sys/time.h>
#endif

using namespace std;

double perfFreq = (double)QPF();
FuncInfos   funcInfos;
#ifdef BDCTHREADED
DWORD curFrame = TlsAlloc();//thread local storage index
#else
TimerFrame* curFrame    = nullptr;//for keeping track of who's a parent of who, set by TimerFrame::enter()
#endif

int64_t QPF()
{
#if defined _WIN32 || defined _WIN64
   int64_t freq;
   QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
   return freq;
#else
   return 1000000;
#endif
}

int64_t QPC()
{
#if defined _WIN32 || defined _WIN64
   int64_t now;
   QueryPerformanceCounter((LARGE_INTEGER*)&now);
   return now;
#else
   timespec t;
   //clock_gettime(CLOCK_MONOTONIC, &t);
   return t.tv_nsec;
#endif
}

double QPCD()
{
   return QPC() / perfFreq;
}

int64_t timeSince(int64_t start)
{
   return QPC() - start;
}

double timeSinceD(int64_t start)
{
   return (QPC() - start) / perfFreq;
}

//------------------------------------------------------------------------------
#ifdef _WIN32
Lock::Lock()
{
   InitializeCriticalSection(&cs);
}
Lock::~Lock()
{
   //DeleteCriticalSection(&cs);
}
void Lock::enter()
{
   EnterCriticalSection(&cs);
}
void Lock::leave()
{
   LeaveCriticalSection(&cs);
}
#endif
//------------------------------------------------------------------------------
Timer::Timer()
{
   total  = 0;
   active = 0;
}
void Timer::start()
{
   if (active == 0)
      started = QPC();
   ++active;
}
void Timer::stop()
{
   --active;
   if (active == 0)
      total += QPC() - started;
}
void Timer::reset()
{
   total = 0;
   active = 0;
}
//------------------------------------------------------------------------------
TimerMult::TimerMult()
{
   active = 0;
   total  = 0;
}
void TimerMult::step()
{
   int64_t now = QPC();
   total += active * (now - changed);
   changed = now;
}
void TimerMult::start()
{
   step();
   ++active;
}
void TimerMult::stop()
{
   step();
   --active;
}
void TimerMult::reset()
{
   active = 0;
   total  = 0;
}
//------------------------------------------------------------------------------
FuncInfo::FuncInfo()
{
   count = 0;
}
void FuncInfo::start()
{
   main.start();
   tree.start();
   ++count;
}
void FuncInfo::pause()
{
   main.stop();
}
void FuncInfo::unpause()
{
   main.start();
}
void FuncInfo::stop()
{
   main.stop();
   tree.stop();
}
void FuncInfo::reset()
{
   main.reset();
   tree.reset();
   count = 0;
}
//------------------------------------------------------------------------------
EventTimer::EventTimer(double secs) : interval(int64_t(secs * perfFreq))
{
}
void EventTimer::start()
{
   started = QPC();
}
bool EventTimer::next()
{
   int64_t now = QPC();
   if (now - started > interval)
   {
      started = now;
      return true;
   }
   else
      return false;
}
//------------------------------------------------------------------------------
#ifdef BDCDISABLETIMING
TimerFSimple::TimerFSimple(const char*)
{
}
TimerFSimple::TimerFSimple(const char*, string)
{
}
TimerFSimple::~TimerFSimple()
{
}
#else
TimerFrameF::TimerFrameF(const char* funcName)
{
   FuncInfos::iterator i = funcInfos.find(funcName);
   if (i != funcInfos.end())
      funcInfo = i->second;
   else
   {
      i = funcInfos.begin();
      while (i != funcInfos.end() && strcmp(funcName, i->second->name))
         ++i;
      if (i != funcInfos.end())
      {
         //insert new const char* (with same name) associated with existing FuncInfo
         funcInfo = i->second;
         funcInfos[funcName] = funcInfo;
      }
      else
      {
         //insert new FuncInfo
         funcInfo = new FuncInfo;
         funcInfo->name = funcName;
      }
   }

   funcInfo->start();
}
TimerFrameF::~TimerFrameF()
{
   funcInfo->stop();
}
#endif
//------------------------------------------------------------------------------
#ifdef BDCDISABLETIMING
TimerFrame::TimerFrame(const char*)
{
}
TimerFrame::TimerFrame(const char*, string)
{
}
TimerFrame::~TimerFrame()
{
}
#else
TimerFrame::TimerFrame(const char* funcName)
{
   enter(funcName);
}
TimerFrame::TimerFrame(const char* funcName, string)
{
   enter(funcName);
}
TimerFrame::~TimerFrame()
{
   exit();
}
#endif
void TimerFrame::enter(const char* funcName)
{
   ended = false;
   static Lock lock;

#ifdef BDCTHREADED
   lock.enter();
#endif
   FuncInfos::iterator i = funcInfos.find(funcName);
   if (i != funcInfos.end())
      funcInfo = i->second;
   else
   {
      i = funcInfos.begin();
      while (i != funcInfos.end() && strcmp(funcName, i->second->name))
         ++i;
      if (i != funcInfos.end())
      {
         //insert new const char* (with same name) associated with existing FuncInfo
         funcInfo = i->second;
         funcInfos[funcName] = funcInfo;
      }
      else
      {
         //insert new FuncInfo
         funcInfo = new FuncInfo;
         funcInfo->name = funcName;
      }
   }
#ifdef BDCTHREADED
   lock.leave();
#endif
   int64_t now = QPC();
#ifdef BDCTHREADED
   parent = (TimerFrame*)TlsGetValue(curFrame);
   TlsSetValue(curFrame, this);
#else
   parent = curFrame;
   curFrame = this;
#endif
   if (parent)
      parent->funcInfo->pause();
   funcInfo->start();
}
void TimerFrame::exit()
{
   if (ended) return;
   ended = true;
   funcInfo->stop();
   if (parent)
      parent->funcInfo->unpause();
#ifdef BDCTHREADED
   TlsSetValue(curFrame, parent);
#else
   curFrame = parent;
#endif
}
void TimerFrame::exit(string)
{
   exit();
}
//------------------------------------------------------------------------------
TimerFrameD::TimerFrameD(const char* funcName) : TimerFrame(funcName)
{
   ODSL(TS+"->" + funcName);
   ++debugLevel;
}
TimerFrameD::TimerFrameD(const char* funcName, string args) : TimerFrame(funcName)
{
   ODSL(TS+"->" + funcName+" "+args);
   ++debugLevel;
}
TimerFrameD::~TimerFrameD()
{
   exit();
}
void TimerFrameD::exit()
{
   if (ended) return;
   TimerFrame::exit();
   --debugLevel;
   ODSL(TS+"<-" + funcInfo->name);
}
void TimerFrameD::exit(string result)
{
   if (ended) return;
   TimerFrame::exit();
   --debugLevel;
   ODSL(TS+"<-" + funcInfo->name + "=" + result);
}
//------------------------------------------------------------------------------
string formatTime(double t)
{
	//int64_t E = log10(T) - 3;
	//T = floor(T / pow(10.0, E) + 0.5) * pow(10.0, E);
	int64_t p = -(floor(log10(t)) - 2) / 3;
	/*
	T       log  prefix   E
	1        0    0       0
	0.9     -0.1  1 m    -1
	0.001   -3    1 m    -3
	0.0009  -3.1  2 u    -4
	*/
	char prefix = " mun"[p];
	char buf[256];
	sprintf_s(buf, 256, "%07.3lf%cs", t * pow(10.0, p*3), prefix);
	return buf;
	//return AnsiString(T * pow10(P*3)) + Prefix + "s";
}

string formatTime(int64_t t)
{
   return formatTime(t / perfFreq);
}

void showTimes()
{
   int64_t total = 0;
   vector<string> names;
   vector<string> times;
   vector<string> timesT;
   vector<string> counts;
   int64_t lnames = 0;
   int64_t ltimes = 0;
   int64_t ltimesT = 0;
   int64_t lcounts = 0;
   for (FuncInfos::iterator i = funcInfos.begin(); i != funcInfos.end(); ++i)
   {
      names .push_back(i->first);
      times .push_back(toString(i->second->main.total      / perfFreq));
      timesT.push_back(toString(i->second->tree.getTotal() / perfFreq));
      counts.push_back(toString(i->second->count));
      //std::cout << i->first << "   t=" << i->second.time / perfFreq << "    n=" << i->second.count << std::endl;
      total += i->second->main.total;
      i->second->reset();
   }
   for (size_t i = 0; i < (int64_t)names.size(); ++i)
   {
      lnames  = max(lnames , (int64_t)names [i].size());
      ltimes  = max(ltimes , (int64_t)times [i].size());
      ltimesT = max(ltimesT, (int64_t)timesT[i].size());
      lcounts = max(lcounts, (int64_t)counts[i].size());
   }
   std::cout << "total time=" << total / perfFreq << std::endl;
   for (size_t i = 0; i < (int64_t)names.size(); ++i)
   {
      std::cout << pad(names[i], lnames)
                << " t="  << pad(times[i], ltimes)
                << " tr=" << pad(timesT[i], ltimesT)
                << " n="  << pad(counts[i], lcounts) << std::endl;
   }
}
