#include "ParserEarley.h"

void Word::insert(State state)
{
   if (!stateSet.contains(state))
   {
      stateSet.insert(state);
      stateVec.push_back(state);
   }
}

bool State::finished()
{
   return this->dot >= dynamic_cast<Seq*>(this->rule)->elems.size();
}

Words parseEarley(string programText, Rule* grammar)
{
   Words words;
   for (unsigned int i = 0; i < programText.size(); ++i)
   {
      words.push_back(Word(programText[i]));
   }
   
   words[0].insert(State(expr, 0, 0));
   for (unsigned int k = 0; k < words.size(); ++k)
   {
      Word &word(words[k]);
      for (unsigned int stateIx = 0; stateIx < word.stateVec.size(); ++stateIx) // S[k] can expand during this loop
      {
         State &state(word.stateVec[stateIx]);
         
         if (!state.finished())
            if (dynamic_cast<NonTerminal*>(state.next()))
               predictor(state, k, words);         // non_terminal
            else
               scanner(state, k, words);             // terminal
         else
            completer(state, k);
      }
   }
   return words;
}

void predictor(State &state, unsigned int k, Words& words)
{
   Alt *alt = dynamic_cast<Alt*>(state.next());
   if (alt)
   {
      for (auto& x : alt->elems)
         words[k].insert(State(x, 0, k));
   }
}

void scanner(State &state, unsigned int k, Words& words)
{
//procedure SCANNER((A → α•aβ, j), k, words)
//   if a ⊂ PARTS_OF_SPEECH(words[k]) then
//      ADD_TO_SET((A → αa•β, j), S[k + 1])
//   Seq *seq = dynamic_cast<Seq*>(state.next());
//   if (seq)
//   {
      words[k + 1].insert(State(state.rule, state.dot + 1, state.origin));
//   }
}

void completer(State &state, unsigned int k)
{
//   procedure COMPLETER((B → γ•, x), k)
//      for each (A → α•Bβ, j) in S[x] do
//         ADD_TO_SET((A → αB•β, j), S[k])
   auto state.rule->parents();
}
