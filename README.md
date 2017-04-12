# Linq
Fast and Simplified C#.Net Linq for C++ !

Install:
- GCC/Clang
  - Add Options -std=c++14 and -I"YourPath"/include to your compilation
- Visual Studio
  - Go to your Project "Properties" -> "C/C++" -> "General"
    - Add "include" folder path in section "Additional Include Directories"
    > example : $(SolutionDir)\include;
  - Go to your Project "Properties" -> "C/C++" -> "Language"
    - Select ISO C++14 in section "C++ Language Standard"

Run Tests:
- Linux
  - make
  - make run
- Windows
  - Create a new project
  - Follow the installation steps
  - Add overhead.cpp to your project
  - Compile and run :)

Usage:
  - #include "linq/linq.h"

Supported operations:
- All
- Select
- SelectMany
- Where
- OrderBy
- GroupBy
- Skip, SkipWhile, Take
- Count, Any
- Sum, Min, Max

Todo:
- Range
- Single, Last
- TakeWhile
- Concat
- Distinct, Union, Intersect, Except
- Contains
