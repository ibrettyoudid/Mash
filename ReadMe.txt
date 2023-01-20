========================================================================
AppWizard uses "TODO:" comments to indicate parts of the source code you
should add to or customize.
/////////////////////////////////////////////////////////////////////////////

TODO:
documentation built into variables and values (both)

build scheme into the text editor, hello modern emacs

uses Any as a variant type

Any creates a copy in the constructor

//=========================================================================

why am i torturing myself making this compatible with c++?
well, there is a lot of libraries written in c
the STL is written in c++
there are a lot of applications written in c++

have to register stack variables with the GC if they point to heap variables
or make a new pointer type that does that automatically

handy to have an interpreter for debugging programs

would probably be a lot less weird in LLVM

i think this way is more fun

//=========================================================================
exact pointer -> struct/tagged struct

inexact pointer -> tagged struct

tagged pointer -> struct/tagged struct

can reinterpret inexact pointer as exact pointer if tag is correct
can reinterpret tagged pointer as exact pointer if tag is correct

can reinterpret exact pointer as inexact pointer if struct is tagged
can convert     exact pointer to tagged pointer



struct Tagged<T>
{
   Type* tag;
   T     payload;
};

struct TagPtr<T>
{
   Type* tag;

};

OR can store tags separately in a map
//don't get too clever here


left recursion removal

e -> e + t
  -> e - t
  -> t

t -> 0..9 t

                
                   
              
  +-e-+-+      
  |   | |      
+-e-+ | |    
| | | | |     
e | | | |         
| | | | | 
t | t | t
3 + 2 - 4


------------------

e -> t z

z -> + t z
z -> - t z
z ->



+e--+
|   |
t +-z---+
| | |   |
| | t +-z-+
| | | | |
| | | | t
3 + 2 - 4