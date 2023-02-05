// Copyright (c) 2023 Brett Curtis
#include "stdafx.h"
#include <fstream>
#pragma hdrstop
#include "Parser.h"
#include "any.h"
#include "Tokens.h"

using namespace std;

LexFile* interactive = new LexFile("<interactive>");
LangD   langD  ;
Haskell haskell;
Scheme  scheme ;

Parser  parser;
Any fromText(string text)
{
   return parser.setProgram(text)->initUnit()->parseExpr();
}

bool operator==(Any any, const char* str)
{
   return toText(any) == str;
}

bool operator!=(Any any, const char* str)
{
   return toText(any) != str;
}

bool operator<(const Op* LHS, const std::string& name)
{
   return LHS->name < name;
}

bool operator<(const std::string& name, const Op* RHS)
{
   return name < RHS->name;
}

Parser* Parser::setFile(LexFile* _file)
{
   file = _file;
   p = _file->b;
   e = _file->e;
   lineNo = 1;
   return this;
}

Parser* Parser::setProgram(string& str)
{
   file = interactive;
   file->text = str;
   file->b = (iter)str.c_str();
   file->e = file->b + str.size();
   p = file->b;
   e = file->e;
   lineNo = 1;
   return this;
}

Parser* Parser::initUnit()
{
   queue.clear();
   done.clear();
   parseEnd = false;
   return this;
}

Parser* Parser::pushFile()
{
   fileStack.push_back(LexFileRef(file, p, lineNo));
   return this;
}

Parser* Parser::popFile()
{
   if (fileStack.size() == 0)
   {
      parseEnd = true;
      return this;
   }
   file   = fileStack.back().file;
   p      = fileStack.back().p;
   e      = file->e;
   lineNo = fileStack.back().lineNo;
   fileStack.pop_back();
   return this;
}

void LexFile::load()
{
   //FILE* f = fopen(name.c_str(), "rb");
   basic_ifstream<char> strm(name.c_str());
   text.resize(strm.tellg());
   strm.seekg(0);
   strm.read(const_cast<char*>(text.c_str()), text.size());
   b      = (iter)text.c_str();
   e      = b + text.size();
   loaded = true;
}

Any Parser::parseError(Any t, string err)
{
   if (file)
   {
      err = string("File '") + file->name + "' line " + lineNo + ": " + err;
   }
   else
   {
      err = string("Interactive: ") + err;
   }
   err = string(typeid(*this).name() + TYPEIDOFF) + ": " + err;
   if (tentative) err = "TENTATIVE:" + err;
   else
      bool Stop = true;
   lastError = done.size();
   ++errors;
   return t;
}

Any Parser::parseExprPrimary()
{
   Any tok = getToken();
   //if (tok.typeInfo == tiInt || tok.typeInfo == tiInt64 || tok.typeInfo == tiFloat || tok.typeInfo == tiDouble || tok.typeInfo == tiSymbol || tok.typeInfo == tiString) return tok;
   return tok;
}

Op* Parser::AsOp(Any v)
{
   if (v.typeInfo != tiSymbol) return nullptr;
   Symbol* sym = &v.as<Symbol>();
   Op tempOp(toText(v));
   return *lang->ops.find(&tempOp);
}

Any Parser::parseExpr(int64_t MinPrec)
{
   Any R = nil;
   Any tok = nextToken();
   if (nextStr() == "(")
   {
      getTokenNoCheck("(");
      R = parseExpr(400);
      getTokenCheck(")");
   }
   else
   {
      Op tempOp(toText(tok));
      auto it = lang->ops.find(&tempOp);
      if (it != lang->ops.end())
      {
         Op* op = *it;
         if (OpFix* opFix = op->fix[ftPre])
         {
            getToken();
            R = mylist(opFix->name, parseExpr(opFix->prec));
         }
         else
         {
            parseError(nullptr, string("expected prefix operator got ") + toText(getToken()));
            return Error(0, 0, /*tok->File, tok->From,*/"error");
         }
      }
      else if (tok == "if")
      {
         getTokenNoCheck("if");
         Any Cond = parseExpr(400);
         getTokenCheck("then");
         Any Then = parseExpr(400);
         getTokenCheck("else");
         Any Else = parseExpr(400);
         R = mylist(tok, Cond, Then, Else);
      }
      else if (tok == "\\")
      {
         getTokenNoCheck("\\");
         Any Args = mylist();
         for (;;)
         {
            Any tok1 = getToken();
            if (tok1 == "->") break;
            else              Args = pushBack(Args, tok1);
         }
         R = mylist(getSymbol("lambda"), Args, parseExpr(400));
      }
      else if (tok == "[")
      {
         int64_t Ellipses = 0;
         getTokenNoCheck("[");
         R = mylist();
         if (nextStr() == "]")
            getTokenNoCheck("]");
         else
            for (;;)
            {
               R = pushBack(R, parseExpr(400));
               Any tok1 = getToken();
               if (tok1 == "]") break;
               else if (tok1 == "..")
               {
                  R = pushBack(R, getSymbol(".."));
                  ++Ellipses;
               }
               else if (tok1 != ",") parseError(nullptr, string("expected , after list element, got") + toText(tok1));
            }
         if (Ellipses)
         {
            const Cons* newr = nil;
            int64_t from = 0;
            for (Cons* c = R; c; c = c->cdr)
            {
               if (isNum(c->car))
               {
                  from = c->car;
                  newr = pushBack(newr, from);
               }
               else if (c->car == "..")
               {
                  c = c->cdr;
                  int64_t to = c->car;
                  for (size_t i = from + 1; i < to; ++i)
                     newr = pushBack(newr, i);
               }
            }
            R = newr;
         }
         R = cons(getSymbol("list"), R);
      }
      else if (tok == "[]")
      {
         getTokenNoCheck("[]");
         R = mylist();
         R = pushBack(R, getSymbol("list"));
      }
      else
      {
         R = parseExprPrimary();
         if (R.typeName() == "Nil")
         {
            parseError(nullptr, string("expected expression got ") + toText(getToken()));
            return Error(0, 0, /*tok->File, tok->From,*/"error");
         }
      }


   }
   bool ListCreated = false;
   while (true)
   {
      Any tok = nextToken();
      if (tok == ")" || tok == "]" || tok == "}" || tok == "," || tok == "then" || tok == "else" || tok == "->" || tok == ":" || tok == ".." || tok == "by" || tok == "for" || tok.typeInfo == tiParseEnd) break;
      Op tempOp(toText(tok));
      auto it = lang->ops.find(&tempOp);
      if (it != lang->ops.end())
      {
         Op*    op    = *it;
         OpFix* opFix = op->fix[ftIn];
         if (opFix && (opFix->prec > MinPrec
                        || (opFix->prec == MinPrec && opFix->assoc == atRight)))
         {
            getToken();
            Any RHS = parseExpr(opFix->prec);
            Any NewR = mylist(getSymbol(op->name), R, RHS);
            if (!opFix->other.empty())
            {
               getTokenCheck(opFix->other);
               NewR = pushBack(NewR, parseExpr(opFix->prec));
            }
            R = NewR;
            ListCreated = false;
            continue;
         }
         opFix = op->fix[ftPost];
         if (opFix && opFix->prec >= MinPrec)
         {
            getToken();
            Any NewR = mylist(getSymbol(op->name), R);
            if (!opFix->other.empty())
            {
               if (nextStr() == opFix->other)
                  getToken();
               else
                  while (true)
                  {
                     NewR = pushBack(NewR, parseExpr(400));
                     tok = getToken();
                     if      (toText(tok) == opFix->other) break;
                     else if (toText(tok) == ","         ) continue;
                     else
                     {
                        parseError(nullptr, TS+"expected , or "+opFix->other+" got "+toText(tok));
                        break;
                     }
                  }
            }
            R = NewR;
            ListCreated = false;
            continue;
         }
         break;
      }
      else if (MinPrec < 800)
      {
         if (!ListCreated)
         {
            R = mylist(R);
            ListCreated = true;
         }
         R = pushBack(R, parseExpr(800));
         continue;
      }
      break;
   }
   return R;
}

Any Parser::parseSExpr()
{
   Any tok = nextToken();
   if (toText(tok) == "(")
      return parseList();
   else if (toText(tok) == "'")
   {
      getToken();
      return mylist(tok, parseSExpr());
   }
   else
      return getToken();
}

Any Parser::parseList()
{
   Any Res = mylist();
   Any B = getTokenCheck("(");
   //List.Ptr->File = B.Ptr->File;
   //List.Ptr->From = B.Ptr->From;
   while (true)
   {
      Any tok = nextToken();
      if (toText(tok) == ")")
      {
         getToken();
         break;
      }
      else if (tok.typeInfo == tiParseEnd)
         break;
      else
         Res = pushBack(Res, parseSExpr());
   }
   return Res;
}

string Parser::nextStr()
{
   return toText(nextToken());
}

void Parser::putToken()
{
   queue.push_front(done.back());
   done.pop_back();
}

Any Parser::getTokenCheck(string Text)
{
   Any tok = getToken();
   if (toText(tok) == Text) return tok;
   parseError(nullptr, "expected " + Text+" got "+toText(tok));
   //return new TError(tok->File, tok->From, tok->GetText());
   return Any();
}

Any Parser::getTokenNoCheck(string Text)
{
   return getToken(false);
}

bool IsSpace(Any V)
{
   return false;
}

Any Parser::getToken(bool Space)
{
   while (true)
   {
      ensure(1);
      Any ParseResult = queue.front();
      if (savedStates) done.push_back(ParseResult);
      queue.pop_front();
      if (Space || !IsSpace(ParseResult))
      {
         //string T = typeid(*this).name() + 7;
         //if (T == "TCPPParser")
         {
            string G = toText(ParseResult);
            if (G.size() < 16)
               G += string(16 - G.size(), ' ');
            G += "Next="+nextStr();
            ODSL("getToken="+G);
         }
         return ParseResult;
      }
   }

}

Any Parser::nextToken(int64_t N)
{
   for (size_t I = 0; ; ++I)
   {
      ensure(I + 1);
      if (!IsSpace(queue[I]))
      {
         //ODS(string(typeid(*this).name())+"->Parser::getToken ="+ParseResult.Text2());
         //ODS(string(typeid(*this).name())+"->Parser::nextToken="+nextToken().Text2());
         if (N == 0) return queue[I];
         --N;
      }
   }
   return Any();
}

void Parser::ensure(int64_t N)
{
   while (N > queue.size())
      queue.push_back(getToken2());
}

Any Parser::getToken2()
{
   Any ParseResult = getToken3();
   //ODS(string(typeid(*this).name())+"->Parser::getToken2="+ParseResult.Text1());
   return ParseResult;
}

#if 1
//! $ % & * + - . / : < = > ? @ ^ _ ~
bool issymbol (TCHAR c) { return strchr("!$%*+-/:<=>?@^_~"   , c) != nullptr; }
bool issymbolh(TCHAR c) { return strchr("!$%*+-/:<=>?@^_~\\.", c) != nullptr; }
bool isident0 (TCHAR c) { return isalpha(c) || issymbol(c); }
bool isident  (TCHAR c) { return isalnum(c) || issymbol(c); }
bool iswhite  (TCHAR c) { return c == ' '   || c == '\t'; }
bool isline   (TCHAR c) { return c == '\n'  || c == '\r'; }
bool iswsnl   (TCHAR c) { return iswhite(c) || isline(c); }

#else
//! $ % & * + - . / : < = > ? @ ^ _ ~
bool issymbol (TCHAR c) { return wcschr(L"!$%*+-/:<=>?@^_~"   , c) != nullptr; }
bool issymbolh(TCHAR c) { return wcschr(L"!$%*+-/:<=>?@^_~\\.", c) != nullptr; }
bool isident0 (TCHAR c) { return iswalpha(c) || issymbol(c); }
bool isident  (TCHAR c) { return iswalnum(c) || issymbol(c); }
bool iswhite  (TCHAR c) { return iswblank(c); }
bool isline   (TCHAR c) { return c == '\n' || c == '\r'; }
bool iswsnl   (TCHAR c) { return iswhite(c) || isline(c); }
#endif

Any Parser::setFilePos(Any Res, iter B)
{
   //Res->From = B - File->B;
   //Res->File = File;
   return Res;
}

Any Parser::getToken3()
{
   if (p == e)
   {
      popFile();
      if (parseEnd) return ParseEnd();
   }
   while (true)
   {
      if (*p == ' ' || *p == '\t')
      {
         ++p;
         while (*p == ' ' || *p == '\t')
         {
            ++p;
         }
         //return new TSpace(File, BN, string(b, p));
      }
      else if (*p == '\n' || *p == '\r')
      {
         if (p < e - 1 && p[0] + p[1] == 23) ++p;
         ++p;
         ++lineNo;
         //ODSL(TS+"Line="+lineNo);
         lineStart = p;
         prep = false;
         //return new TLine(File, BN, string(b, p));
      }
      /*
      if (p < e - 1 && p[0] == '\\' && (p[1] == '\n' || p[1] == '\r'))
      {
         ++p;
         if (p < e - 1 && p[0] + p[1] == 23) ++p;
         ++p;
         ++LineNo;
         LineStart = p;
         return new TSoftLine(File, BN, string(b, p));
      }
      if (p < e - 1 && p[0] == '/' && p[1] == '*')
      {
         p += 2;
         while (p < e - 1 && !(p[0] == '*' && p[1] == '/'))
         {
            ++p;
            if (*p == '\n' || *p == '\r')
            {
               if (p < e - 1 && p[0] + p[1] == 23) ++p;
               ++p;
               ++LineNo;
               ODSL(TS+"Line="+LineNo);
               LineStart = p;
               Prep = false;
            }
         }
         p += 2;
         return new TComment(File, BN, string(b, p));
      }
      */
         else if (p < e - 1 && p[0] == '#' && p[1] == '|')
         {
            p += 2;
            int64_t Depth = 1;
            while (p < e)
            {
               if (p < e - 1 && p[0] == '#' && p[1] == '|')
               {
                  p += 2;
                  ++Depth;
               }
               else if (p < e - 1 && p[0] == '|' && p[1] == '#')
               {
                  p += 2;
                  --Depth;
                  if (Depth == 0) break;
               }
               else
                  ++p;
            }
            //return new TComment(File, BN, string(b, p));
         }
      else if (*p == ';')
      {
         while (p < e && *p != '\n' && *p != '\r') ++p;
         if (p < e - 1 && p[0] + p[1] == 23) ++p;
         ++p;
         ++lineNo;
         //return new TComment(File, BN, string(b, p));
      }
      else
         break;
   }
   /*
   if (*p == '#')
   {
      ++p;
      if (p < e && *p == '#') ++p;
      Prep = true;
      return new TPunc(File, BN, string(b, p));
   }
   */
   iter b = p;
   int64_t  BN = b - file->b;

   if (*p == '(' || *p == '{' || *p == '[' || *p == '}' || *p == ')' || *p == ']' || *p == ',' || (lang == &scheme && (*p == '\'' || *p == '`')))
   {
      string str(b, ++p);
      Symbol* sym = getSymbol(str);
      return sym;
   }
   if (lang == &haskell)
   {
      /*
      p = b + 4;
      if (p > e) p = e;
      for (; p > b; --p)
      {
         string s(b, p);
         Op* op = Lang->Ops.findx(s).val();
         if (op)
         {
            ODST(ToText(op));
            return SetF(Create(TSymbol(op)), b);
         }
      }
      */
      if (isalpha(*p))
      {
         do ++p; while (p < e && isalnum(*p));
         return setFilePos(getSymbol(string(b, p)), b);
      }

      if (issymbolh(*p))
      {
         do ++p; while (p < e && issymbolh(*p));
         return setFilePos(getSymbol(string(b, p)), b);
      }
   }
   else if (lang == &scheme)
   {
      if (isident0(*p))
      {
         do ++p; while (p < e && isident(*p));
         string str(b, p);
         /*
         if (Op* op = Lang->Ops.find(&Op(Str)))                             return new LexRef(File, BN, Str, op);
         if (TKeyword* Keyword = Lang->Keywords.find(&TKeyword(Str)).val())   return new LexRef(File, BN, Str, Keyword);
         */
         return getSymbol(str);
      }
   }
   if (*p >= '0' && *p <= '9' || *p == '.')
   {
      string n;
      bool dot = false;
      bool exp = false;
      bool digit = false;
      while (p < e)
      {
         if (*p >= '0' && *p <= '9')
         {
            n += *p++;
            digit = true;
         }
         else if (*p == '.')
         {
            if (dot || exp) break;
            dot = true;
            n += *p++;
         }
         else if (*p == 'e' || *p == 'E')
         {
            if (exp || !digit) break;
            string n1 = n;
            iter    p1 = p;
            ++p;
            n += 'e';
            if (p < e && (*p == '+' || *p == '-')) n += *p++;
            if (p < e && (*p >= '0' && *p <= '9'));
            else
            {
               p = p1;
               n = n1;
               break;
            }
            exp = true;
            digit = false;
         }
         else
            break;
      }
      if (n == ".")
         p = b;
      else if (dot || exp)
         return fromString<double>(n);
      else
         return fromString<int64_t>(n);
   }
   if (*p == '"')// || *p == '\'')
   {
      char c = *p;
      string s;
      ++p;
      while (true)
      {
         if (p >= e || *p == c) break;
         if (*p == '\\')
         {
            ++p;
            switch (*p)
            {
               case 'a':	s += '\a';	break;
               case 'b':	s += '\b';	break;
               case 'e':   s += char(27); break;
               case 'f':	s += '\f';	break;
               case 'n':	s += '\n';	break;
               case 'r':	s += '\r';	break;
               case 't':	s += '\t';	break;
               case 'v':	s += '\v';	break;
               case '\'':	s += '\'';	break;
               case '"':	s += '"';	break;

               case 10:
               case 13:
                  while (p < e && (*p == 10 || *p == 13))
                     ++p;
                  break;
               case '0':	case '1':	case '2':	case '3':	case '4':	case '5':	case '6':	case '7':
               {
                  int64_t n = 0;
                  while (p < e && *p >= '0' && *p <= '7')
                     n = n * 8 + (*p++ - '0');
                  s += (char)n;
                  break;
               }
               case 'x':
               {
                  int64_t n = 0;
                  for (;;)
                  {
                     if (p >= e) break;
                          if (*p >= '0' && *p <= '9') n = n * 16 + (*p++ - '0');
                     else if (*p >= 'a' && *p <= 'f')	n = n * 16 + (*p++ - 'a') + 10;
                     else if (*p >= 'A' && *p <= 'F') n = n * 16 + (*p++ - 'A') + 10;
                     else break;
                  }
                  s += char(n);
                  break;
               }
               default:
                  s += *p;
                  break;
            }
         }
         else
         {
            s += *p;
            ++p;
         }
      }
      if (p < e) ++p;
      //throw TTypeError();
      if (c == '"') return s; else return s[0];
   }
   parseError(nullptr, string("Illegal char ") + *p);
   ++p;
   return nullptr;
}

void Parser::loadState(TState State)
{
   int64_t undo = (int64_t)done.size() - State.nDone;
   if (undo > 0)
   {
      queue.insert(queue.begin(), done.end() - undo, done.end());
      done.erase(                 done.end() - undo, done.end());
   }
   else if (undo < 0)
   {
      done.insert(done.end(), queue.begin(), queue.begin() - undo);
      queue.erase(            queue.begin(), queue.begin() - undo);
   }
   //ODSL("BACKTRACKED... nextToken="+nextStr());
}

int64_t Parser::getLineNo()
{
   return lineNo;
}

void Language::addOp(OpFix* opFix)
{
   opFixs.insert(opFix);
   Op* op = new Op(opFix);
   op->fix[opFix->fix] = opFix;
   ops.insert(op);
   //++op->RefCount;
   //++opFix->RefCount;
}

LangD::LangD()
{
   addOp(new OpFix("::"   , "" , ftIn  , atRight, 660, OPscope      ));

   addOp(new OpFix("."    , "" , ftIn  , atLeft , 650, OPmem        ));
   addOp(new OpFix("->"   , "" , ftIn  , atLeft , 650, OPmem_ptr    ));
   addOp(new OpFix("("    , ")", ftPost, atLeft , 640, OPfunc       ));
   addOp(new OpFix("["    , "]", ftPost, atLeft , 640, OPindex      ));
// addOp(new OpFix("!("   , ")", ftPost, atLeft , 640, OPtemplate   ));

   addOp(new OpFix("!"    , "" , ftPre , atRight, 630, OPnot_bool   ));
   addOp(new OpFix("~"    , "" , ftPre , atRight, 630, OPnot_bit    ));
   addOp(new OpFix("+"    , "" , ftPre , atRight, 630, OPadd_unary  ));
   addOp(new OpFix("-"    , "" , ftPre , atRight, 630, OPsub_unary  ));
   addOp(new OpFix("++"   , "" , ftPre , atRight, 630, OPinc_pre    ));
   addOp(new OpFix("--"   , "" , ftPre , atRight, 630, OPdec_pre    ));
   addOp(new OpFix("++"   , "" , ftPost, atRight, 630, OPinc_post   ));
   addOp(new OpFix("--"   , "" , ftPost, atRight, 630, OPdec_post   ));
   addOp(new OpFix("&"    , "" , ftPre , atRight, 630, OPref        ));
   addOp(new OpFix("*"    , "" , ftPre , atRight, 630, OPderef      ));

   addOp(new OpFix(".*"   , "" , ftIn  , atLeft , 620, OPptr_mem    ));
   addOp(new OpFix("->*"  , "" , ftIn  , atLeft , 620, OPptr_mem_ptr));

   addOp(new OpFix("*"    , "" , ftIn  , atLeft , 610, OPmul        ));
   addOp(new OpFix("/"    , "" , ftIn  , atLeft , 610, OPdiv        ));
   addOp(new OpFix("%"    , "" , ftIn  , atLeft , 610, OPmod        ));

   addOp(new OpFix("+"    , "" , ftIn  , atLeft , 600, OPadd        ));
   addOp(new OpFix("-"    , "" , ftIn  , atLeft , 600, OPsub        ));
   addOp(new OpFix("~"    , "" , ftIn  , atLeft , 600, OPcat        ));

   addOp(new OpFix("<"    , "" , ftIn  , atLeft , 590, OPless       ));
   addOp(new OpFix("<="   , "" , ftIn  , atLeft , 590, OPless_equal ));
   addOp(new OpFix(">"    , "" , ftIn  , atLeft , 590, OPmore       ));
   addOp(new OpFix(">="   , "" , ftIn  , atLeft , 590, OPmore_equal ));
   addOp(new OpFix("<<"   , "" , ftIn  , atLeft , 590, OPshl        ));
   addOp(new OpFix(">>"   , "" , ftIn  , atLeft , 590, OPshr        ));

   addOp(new OpFix("=="   , "" , ftIn  , atLeft , 580, OPequal      ));
   addOp(new OpFix("!="   , "" , ftIn  , atLeft , 580, OPnot_equal  ));

   addOp(new OpFix("&"    , "" , ftIn  , atLeft , 570, OPand_bit    ));
   addOp(new OpFix("^"    , "" , ftIn  , atLeft , 560, OPxor_bit    ));
   addOp(new OpFix("|"    , "" , ftIn  , atLeft , 550, OPor_bit     ));
   addOp(new OpFix("&&"   , "" , ftIn  , atLeft , 540, OPand_bool   ));
   addOp(new OpFix("||"   , "" , ftIn  , atLeft , 530, OPor_bool    ));
   addOp(new OpFix("?"    , ":", ftIn  , atLeft , 520, OPcond_quest ));

   addOp(new OpFix("="    , "" , ftIn  , atRight, 500, OPassign     ));
   addOp(new OpFix("*="   , "" , ftIn  , atRight, 500, OPmul_assign ));
   addOp(new OpFix("/="   , "" , ftIn  , atRight, 500, OPdiv_assign ));
   addOp(new OpFix("%="   , "" , ftIn  , atRight, 500, OPmod_assign ));
   addOp(new OpFix("+="   , "" , ftIn  , atRight, 500, OPadd_assign ));
   addOp(new OpFix("-="   , "" , ftIn  , atRight, 500, OPsub_assign ));
   addOp(new OpFix("&="   , "" , ftIn  , atRight, 500, OPand_assign ));
   addOp(new OpFix("|="   , "" , ftIn  , atRight, 500, OPor_assign  ));
   addOp(new OpFix("^="   , "" , ftIn  , atRight, 500, OPxor_assign ));
   addOp(new OpFix("~="   , "" , ftIn  , atRight, 500, OPcat_assign ));
   addOp(new OpFix("<<="  , "" , ftIn  , atRight, 500, OPshl_assign ));
   addOp(new OpFix(">>="  , "" , ftIn  , atRight, 500, OPshr_assign ));

   addOp(new OpFix(","    , "" , ftIn  , atRight, 200, OPcomma      ));
}

Haskell::Haskell()
{
/*
optable   = [ doinfixr 9 [".", "!!"]
            , doinfixr 8 ["^", "^^", "**"]
            , doinfixl 7 ["*", "/"] --, `quot`, `rem`, `div`, `mod`
            , doinfixl 6 ["+", "-"]
            , doinfixr 5 [":", "++"]
            , doinfix  4 ["==", "/=", "<", "<=", ">=", ">"]
            , doinfixr 3 ["&&"]
            , doinfixr 2 ["||"]
            , doinfixl 1 [">>", ">>="]
            , doinfixr 1 ["=<<"]
            , doinfixr 0 ["$", "$!"] --, `seq`
            ]
*/
   addOp(new OpFix("@"    , "" , ftIn  , atRight, 680, OPscope      ));
   //addOp(new OpFix(":"    , "" , ftIn  , atRight, 670, OPscope      ));
   //addOp(new OpFix("::"   , "" , ftIn  , atRight, 660, OPscope      ));

   addOp(new OpFix("."    , "" , ftIn  , atRight, 630, OPmem        ));
   addOp(new OpFix("!!"   , "" , ftIn  , atRight, 630, OPgetElem    ));

   addOp(new OpFix("^"    , "" , ftIn  , atLeft , 620, OPpow        ));

   addOp(new OpFix("*"    , "" , ftIn  , atLeft , 610, OPmul        ));
   addOp(new OpFix("/"    , "" , ftIn  , atLeft , 610, OPdiv        ));
   addOp(new OpFix("%"    , "" , ftIn  , atLeft , 610, OPmod        ));

   addOp(new OpFix("+"    , "" , ftIn  , atLeft , 600, OPadd        ));
   addOp(new OpFix("-"    , "" , ftIn  , atLeft , 600, OPsub        ));

   addOp(new OpFix(":"    , "" , ftIn  , atLeft , 600, OPcons       ));
   addOp(new OpFix("++"   , "" , ftIn  , atLeft , 600, OPcat        ));

   addOp(new OpFix("<"    , "" , ftIn  , atLeft , 590, OPless       ));
   addOp(new OpFix("<="   , "" , ftIn  , atLeft , 590, OPless_equal ));
   addOp(new OpFix(">"    , "" , ftIn  , atLeft , 590, OPmore       ));
   addOp(new OpFix(">="   , "" , ftIn  , atLeft , 590, OPmore_equal ));
   //addOp(new OpFix("<<"   , "" , ftIn  , atLeft , 590, OPshl        ));
   //addOp(new OpFix(">>"   , "" , ftIn  , atLeft , 590, OPshr        ));

   addOp(new OpFix("=="   , "" , ftIn  , atLeft , 580, OPequal      ));
   addOp(new OpFix("/="   , "" , ftIn  , atLeft , 580, OPnot_equal  ));

   //addOp(new OpFix("&"    , "" , ftIn  , atLeft , 570, OPand_bit    ));
   //addOp(new OpFix("^"    , "" , ftIn  , atLeft , 560, OPxor_bit    ));
   //addOp(new OpFix("|"    , "" , ftIn  , atLeft , 550, OPor_bit     ));
   addOp(new OpFix("&&"   , "" , ftIn  , atLeft , 540, OPand_bool   ));
   
   addOp(new OpFix("||"   , "" , ftIn  , atLeft , 530, OPor_bool    ));
   //addOp(new OpFix("?"    , ":", ftIn  , atLeft , 520, OPcond_quest ));

   //addOp(new OpFix("="    , "" , ftIn  , atRight, 500, OPassign     ));
   //addOp(new OpFix("*="   , "" , ftIn  , atRight, 500, OPmul_assign ));
   //addOp(new OpFix("/="   , "" , ftIn  , atRight, 500, OPdiv_assign ));
   //addOp(new OpFix("%="   , "" , ftIn  , atRight, 500, OPmod_assign ));
   //addOp(new OpFix("+="   , "" , ftIn  , atRight, 500, OPadd_assign ));
   //addOp(new OpFix("-="   , "" , ftIn  , atRight, 500, OPsub_assign ));
   //addOp(new OpFix("&="   , "" , ftIn  , atRight, 500, OPand_assign ));
   //addOp(new OpFix("|="   , "" , ftIn  , atRight, 500, OPor_assign  ));
   //addOp(new OpFix("^="   , "" , ftIn  , atRight, 500, OPxor_assign ));
   //addOp(new OpFix("~="   , "" , ftIn  , atRight, 500, OPcat_assign ));
   //addOp(new OpFix("<<="  , "" , ftIn  , atRight, 500, OPshl_assign ));
   //addOp(new OpFix(">>="  , "" , ftIn  , atRight, 500, OPshr_assign ));

   addOp(new OpFix(">>"    , "" , ftIn  , atLeft , 200, OPshr          ));
   addOp(new OpFix(">>="   , "" , ftIn  , atLeft , 300, OPshr_assign   ));

   addOp(new OpFix("$"     , "" , ftIn  , atRight, 250, OPapply        ));
   //addOp(new OpFix(","    , "" , ftIn  , atRight, 200, OPcomma      ));
}

Scheme::Scheme()
{
   getSymbol("list",    1, static_cast<int64_t>(Keyword::list   ));
   getSymbol("if",      1, static_cast<int64_t>(Keyword::_if    ));
   getSymbol("if1",     1, static_cast<int64_t>(Keyword::_if1   ));
   getSymbol("let",     1, static_cast<int64_t>(Keyword::let    ));
   getSymbol("let1",    1, static_cast<int64_t>(Keyword::let1   ));
   getSymbol("define",  1, static_cast<int64_t>(Keyword::define ));
   getSymbol("define1", 1, static_cast<int64_t>(Keyword::define1));
   getSymbol("lambda",  1, static_cast<int64_t>(Keyword::lambda ));
   getSymbol("apply1",  1, static_cast<int64_t>(Keyword::apply1 ));
   getSymbol("applyTo", 1, static_cast<int64_t>(Keyword::applyTo));
   getSymbol("begin",   1, static_cast<int64_t>(Keyword::begin  ));
   getSymbol("pop",     1, static_cast<int64_t>(Keyword::pop    ));
   getSymbol("callcc",  1, static_cast<int64_t>(Keyword::callCC ));
   /*
   getSymbol("let", 1, 1);
   getSymbol("let", 1, 1);
   getSymbol("let", 1, 1);
   */
}

string ToText(OpFix* opFix)
{
   if (opFix == nullptr) return "nullptr";
   return TS+"name="+opFix->name+" Fix="+(int64_t)opFix->fix+" prec="+opFix->prec+" Assoc="+(int64_t)opFix->assoc;
}

string ToText(Op* op)
{
   return TS+"name="+op->name+" Pre=["+ToText(op->fix[0])+"] In=["+ToText(op->fix[1])+"] Post=["+ToText(op->fix[2])+"]";
}

/*
(blah (bler (blug (rarh 4) (cupid 5) (stupid 7)

(blah (bler (blug (rarh 4)
                  (cupid 5)
                  (stupid 7))
            (moop (foop shoop))

let x = 4
    y = 5
     + 6

=>

let x = 4; y = 5 + 6

do x = 3
    + 2
   y = 4
    + 5
z = 4

=>

do x = 3+2; y = 4+5
*/
