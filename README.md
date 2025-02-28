# Mash

(requires MSVC)

An interpreter written in C++

The language is a mash up of other programming languages (Scheme, Lisp, Haskell, C++)

It defines a type 'Any' that can hold any other type, including functions

you can use it as a library to do this

int add(int x, int y)
{
   return x+y;
}

int main(int argc, char** argv)
{
   Any f = add;
   cout << f(2, 3) << endl;

   return 0;
}

or you can use it directly. It has Haskell-like syntax

