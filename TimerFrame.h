// Copyright (c) 2023 Brett Curtis
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <string.h>
#endif
#include <map>
#include <iostream>
#include <algorithm>
#include <string>


#ifdef _DEBUG
#define IFDEBUG(x,y) x,y
#else
#define IFDEBUG(x,y)
#endif

extern double perfFreq;

int64_t QPF();
int64_t QPC();
double QPCD();


struct Lock
{
#ifdef _WIN32
   CRITICAL_SECTION cs;
   Lock();
   ~Lock();
   void enter();
   void leave();
#else
   void enter() {}
   void leave() {}
#endif
};

struct Timer
{
   int64_t  started;
   int64_t  total;
   int64_t  active;
   Timer();
   void start();
   void stop();
   void reset();
};

struct TimerMult//call start twice without stop and it counts double time
{
   int64_t  changed;
   int64_t  active;
   int64_t  total;
   TimerMult();
   void step();
   void start();
   void stop();
   void reset();
};

#ifdef _WIN32
template <class Timer>
struct TimerThread
{
   DWORD tlsIndex;

   typedef std::map<DWORD, Timer*> Threads;
   Threads threads;
   Lock    lock;
   
   TimerThread()
   {
      tlsIndex = TlsAlloc();
   }
   void start()
   {
      Timer* timer = (Timer*)TlsGetValue(tlsIndex);
      if (timer == nullptr)
      {
         lock.enter();
         timer = new Timer;
         TlsSetValue(tlsIndex, timer);
         threads[GetCurrentThreadId()] = timer;
         lock.leave();
      }
      timer->start();
   }
   void stop()
   {
      static_cast<Timer*>(TlsGetValue(tlsIndex))->stop();
   }
   int64_t getTotal()
   {
      int64_t t = 0;
      for (typename Threads::iterator i = threads.begin(); i != threads.end(); ++i)
         t += i->second->total;
      return t;
   }
   void reset()
   {
      for (typename Threads::iterator i = threads.begin(); i != threads.end(); ++i)
         i->second->reset();
   }
};
#else
template <class Timer>
struct TimerThread
{
   TimerThread()
   {
   }
   void start()
   {
   }
   void stop()
   {
   }
   int64_t getTotal()
   {
      return 0;
   }
   void reset()
   {
   }
};
#endif
struct FuncInfo
{
   TimerMult            main;//time each thread spends in a func
   TimerThread<Timer>   tree;//time each thread spends in a func or funcs called by it

   int64_t         count;   //total number of calls
   const char*       name;
   FuncInfo();
   void start();
   void pause();
   void unpause();
   void stop();
   void reset();
};

struct EventTimer
{
   int64_t     started;
   int64_t     interval;
               EventTimer    (double secs);
   void start();
   bool next();
};

struct TimerFrameF
{
   FuncInfo*      funcInfo;

            TimerFrameF (const char* funcName);
            ~TimerFrameF();
};

struct TimerFrame
{
   FuncInfo*      funcInfo;
   TimerFrame*    parent;

   bool     ended;

            TimerFrame  (const char* funcName);
            TimerFrame  (const char* funcName, std::string);
            ~TimerFrame ();
   void     enter       (const char* funcName);
   void     exit        (std::string);
   void     exit        ();
   double   elapsed     ();
};

struct TimerFrameD : public TimerFrame
{
   bool     showArgs;

            TimerFrameD (const char* funcName);
            TimerFrameD (const char* funcName, std::string args);
            ~TimerFrameD();
   void     exit        ();
   template <typename T>
   T        exit        (T result);
};

template <typename T>
T TimerFrameD::exit(T result)
{
   if (ended) return result;
   TimerFrame::exit();
   --debugLevel;
   ODSL(TS+"<-" + funcInfo->name + "=" + toText(result));
   return result;
}

struct StrCmp
{
   bool operator()(const char* lhs, const char* rhs) const
   {
      return strcmp(lhs, rhs) < 0;
   }
};

typedef std::map<const char*, FuncInfo*> FuncInfos;
extern FuncInfos   funcInfos;
#ifdef BDCTHREADED
extern DWORD curFrame;
#else
extern TimerFrame* curFrame;//for keeping track of who's a parent of who, set by TimerFrame::enter()
#endif

std::string formatTime(double t);
std::string formatTime(int64_t t);
void showTimes();
int64_t timeSince(int64_t start);
double  timeSinceD(int64_t start);
