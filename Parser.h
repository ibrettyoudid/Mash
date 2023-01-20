// Copyright (c) 2023 Brett Curtis
#ifndef ParserH
#define ParserH
#include <deque>

#include "classes.h"

#define TYPEIDOFF 7

#ifndef _MSC_VER
typedef wchar_t TCHAR;
#endif

typedef const TCHAR* iter;
struct TState;
typedef int TPos;
struct LexFile
{
   std::string   name;
   std::string   text;
   bool     loaded;
   iter     b;
   iter     e;

            LexFile (std::string N = "") : name(N), loaded(false) {}
   void     load    ();
};

struct LexFileRef
{
   LexFile*     file;
   iter         p;
   int         lineNo;
   LexFileRef(LexFile* file, iter p, int _lineNo) : file(file), p(p), lineNo(_lineNo) {}
};

struct Lex      : public Symbol
{
   Lex(LexFile* _file, TPos _from, std::string name)
      : Symbol(name) { }//File = _File; From = _From; }
};
/*
struct LexBase
{
            LexFile*   File;
            TPos        From;
                        LexBase    (LexFile* _File, TPos _From)
                                    : File(_File), From(_From) {}
   virtual  TObjType    ObjType     () { return ELex       ; }
};
struct Lex      : public TNamedObj
{
            LexFile*   File;
            TPos        From;
                        Lex        (LexFile* _File, TPos _From, std::string Name)
                         : TNamedObj(Name),
                           File(_File), From(_From)
                           {}
            TObjType    ObjType     () { return ELex       ; }
};
*/


struct Punc     : public Lex      {  Punc          (LexFile* _File, TPos _From, std::string Name) : Lex     (_File, _From, Name) {} };
struct AnySpace : public Lex      {  AnySpace      (LexFile* _File, TPos _From, std::string Name) : Lex     (_File, _From, Name) {} };
struct Space    : public AnySpace {  Space         (LexFile* _File, TPos _From, std::string Name) : AnySpace(_File, _From, Name) {} };
struct Line     : public AnySpace {  Line          (LexFile* _File, TPos _From, std::string Name) : AnySpace(_File, _From, Name) {} };
struct SoftLine : public AnySpace {  SoftLine      (LexFile* _File, TPos _From, std::string Name) : AnySpace(_File, _From, Name) {} };
struct Comment  : public AnySpace {  Comment       (LexFile* _File, TPos _From, std::string Name) : AnySpace(_File, _From, Name) {} };
struct Error    : public Lex      {  Error         (LexFile* _File, TPos _From, std::string Name) : Lex     (_File, _From, Name) {} };
struct Language;
struct LexRef   : public Lex
{
   void*       Ref;
               LexRef     (LexFile* _File, TPos _From, std::string Name, void* _Ref)
                              : Lex     (_File, _From, Name), Ref(_Ref) {}
};
struct Ident    : public LexRef
{
   Ident      (LexFile* _File, TPos _From, std::string Name, void* _Ref = nullptr)
                  : LexRef  (_File, _From, Name, _Ref) {}
   Ident      (std::string Name, void* _Ref = nullptr)
                  : LexRef  (nullptr, 0, Name, _Ref) {}
};
struct ParseEnd : public Lex      {           ParseEnd() : Lex(nullptr, 0, "$$$ParseEnd$$$") {}};
enum FixType
{
   ftPre ,
   ftIn  ,
   ftPost
};

enum AssocType { atLeft, atRight };

struct OpFix : public SymbolData
{
   FixType     fix;
   int         prec;
   AssocType   assoc;
   std::string      other;
               OpFix   (std::string   _name, std::string _other = "", FixType _fix = ftIn, AssocType _assoc = atLeft, int _prec = 0, int id = 0)
                           : SymbolData(_name, id), other(_other), fix(_fix), assoc(_assoc), prec(_prec) {}
   struct Compare
   {
      bool operator()(const std::string& lhs, const std::string& rhs) const
      {
         //if (lhs )
         return false;
      }
      bool operator()(const OpFix* LHS, const OpFix* rhs) const
      {
         if (LHS->name < rhs->name) return true;
         if (LHS->name > rhs->name) return false;
         if (LHS->fix  < rhs->fix ) return true;
         if (LHS->fix  > rhs->fix ) return false;
         return false;
      }
      bool operator()(const OpFix* LHS, const std::string& name) const
      {
         if (LHS->name < name) return true;
         if (LHS->name > name) return false;
         return false;
      }
      //this requires UNSORTED SEARCH: findxu
      bool operator()(const OpFix* LHS, int id) const
      {
         if (LHS->id < id) return true;
         if (LHS->id > id) return false;
         return false;
      }
   };
};

struct Op : public SymbolData
{
   OpFix*   fix[3];

       Op            (OpFix*      opFix) : SymbolData(opFix->name, opFix->id) { fix[0] = nullptr; fix[1] = nullptr; fix[2] = nullptr; }
       Op            (std::string name ) : SymbolData(name                  ) { fix[0] = nullptr; fix[1] = nullptr; fix[2] = nullptr; }
       ~Op           () {}
   Op* asOp          () { return this; }

   struct Less
   {
      bool operator()(const std::string& lhs, const std::string& rhs) const
      {
         //if (lhs )
         return false;
      }
      bool operator()(const Op* lhs, const Op* rhs) const
      {
         return lhs->name < rhs->name;
      }
      bool operator()(const Op* lhs, const std::string& name) const
      {
         return lhs->name < name;
      }
      bool operator()(const std::string& name, const Op* rhs) const
      {
         return name < rhs->name;
      }
      //this requires UNSORTED SEARCH: findxu
      int operator()(const OpFix* LHS, int id) const
      {
         if (LHS->id < id) return -1;
         if (LHS->id > id) return  1;
         return 0;
      }
   };
};
/*
template <class T>
T ident_cast(AnyP* O)
{
   T  O1 =        dynamic_cast<T       >(O);
   if (O1) return O1;
   LexRef* L =   dynamic_cast<LexRef*>(O);
   if (L ) return dynamic_cast<T       >(L->Ref);
   return nullptr;
}
*/
struct Parser
{
   typedef std::deque<Any>   Queue;
   std::vector<Any>          done;
   Queue                     queue;
   std::vector<LexFileRef>   fileStack;
   std::vector<LexFile*>     files;
   std::vector<LexFile*>     includeDirs;

   std::string      fileName;
   LexFile*    file;
   iter        lastP;
   iter        p;
   iter        e;
   iter        lineStart;
   int         lineNo;
   int         savedStates;
   bool        parseEnd;
   bool        prep;
   bool        include;
   int         tentative;
   int         errors;
   int         lastError;
   Language*   lang;

   Parser() : savedStates(0), tentative(0) {}

   virtual Parser*  setFile           (LexFile* f);
   virtual Parser*  setProgram        (std::string& str);
           Parser*  initUnit          ();
   virtual Parser*  pushFile          ();
   virtual Parser*  popFile           ();
   virtual Op*      AsOp              (Any V);
   virtual Any      parseExprPrimary  ();
   virtual Any      parseExpr         (int minPrec = 0);
   virtual Any      parseSExpr        ();
   virtual Any      parseList         ();
   virtual Any      parseError        (Any t, std::string err);
   virtual std::string   nextStr           ();
   virtual Any      nextToken         (int N = 0);
   virtual void     putToken          ();
   virtual Any      getTokenCheck     (std::string Text);
   virtual Any      getTokenNoCheck   (std::string Text);
   virtual Any      getToken          (bool space = false);
   virtual void     ensure            (int n);
   virtual Any      getToken2         ();
   virtual Any      setFilePos              (Any v, iter b);
   virtual Any      getToken3         ();
   virtual void     loadState         (TState State);
//                     Parser            () : NextToken(MakeProp(&GetNextToken)), NextStr(MakeProp(&GetNextStr)), NextType(MakeProp(&GetNextType)) {}
   virtual  int      getLineNo         ();
/*
   template <class T> T DCastError        (TObj* Obj)
   {
      T Val = dynamic_cast<T>(Obj);
      if (Val) return Val;
      ParseError(nullptr, std::string()+"expected "+(typeid(T).name() + 7)+" got "+(typeid(*Obj).name() + 7));
      ODSL(std::string("Returning nullptr\n"));
      return 0;
   }
   template <class T> T ident_cast_error  (TObj* Obj)
   {
      T Val = ident_cast<T>(Obj);
      if (Val) return Val;
      ParseError(nullptr, std::string()+"expected "+(typeid(T).name() + 7)+" got "+(typeid(*Obj).name() + 7));
      ODSL(std::string("Returning nullptr\n"));
      return 0;
   }
*/
};

struct TState
{
   Parser*     parser;
   int         nDone;
   TState() : parser(nullptr), nDone(0)
   {
   }
   TState(Parser *L) : parser(L), nDone(parser->done.size())
   {
      ++parser->savedStates;
   }
   ~TState()
   {
      --parser->savedStates;
      if (parser->savedStates == 0) parser->done.clear();
   }
   TState(const TState& rhs)
   {
      parser = rhs.parser;
      nDone = rhs.nDone;
      ++parser->savedStates;
   }
   TState&     operator=(const TState& rhs)
   {
      if (parser == nullptr)
      {
         parser = rhs.parser;
         if (parser)
            ++parser->savedStates;
      }
      else if (parser != rhs.parser)
         throw "must be same Parser!";
      nDone = rhs.nDone;
      return *this;
   }
};

bool isident0(char C);
bool isident(char C);
bool iswhite(char C);
bool isline(char C);
bool iswsnl(char C);
/*
int operator<(const Op* LHS, const Op* rhs)
{
   if (LHS->name < rhs->name) return -1;
   if (LHS->name > rhs->name) return 1;
   return 0;
}
*/

typedef  std::set<Op   *, Op::Less   > Ops   ;
typedef  std::set<OpFix*, OpFix::Compare> OpFixs;

struct Language
{
            Ops         ops;
            OpFixs      opFixs;
//            TBlock      BuiltIn;
            virtual    ~Language() {}
            void        addOp   (OpFix* opFix);
};

struct LangCPP : public Language
{
   LangCPP();
};

struct LangD : public Language
{
   LangD();
};

struct Haskell : public Language
{
   Haskell();
};

enum class Keyword : int
{
   list,
   _if,
   _if1,
   let,
   let1,
   define,
   define1,
   lambda,
   apply1,
   applyTo,
   begin,
   pop,
   callCC
};

struct Scheme : public Language
{
   Scheme();
};

extern   LangD    langD;
extern   Haskell  haskell;
extern   Scheme   scheme;
extern   Parser   parser;

Any fromText(std::string text);
//---------------------------------------------------------------------------
#endif
