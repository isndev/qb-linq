# Linq

Fast and Simplified C#.Net Linq for C++ !

### Install

1 - GCC/Clang

- add Options -std=c++14 and -I"YourPath"/include to your compilation

2 - Visual Studio

  - Go to your Project "Properties" -> "C/C++" -> "General"
  - Add "include" folder path in section "Additional Include Directories" `e.g: $(SolutionDir)\include`
  - Go to your Project "Properties" -> "C/C++" -> "Language"
  - Select ISO C++14 in section "C++ Language Standard"

### Run Tests

Linux
```bash
  $> make && make run
```

Windows
  - Create a new project
  - Follow the installation steps
  - Add overhead.cpp to your project
  - Compile and run :)

### Usage

```cpp
  #include "linq/linq.h"
```

#### Supported operations

- All
- Select
- SelectMany
- Where
- OrderBy
- GroupBy
- Skip, SkipWhile
- Take, TakeWhile
- Each
- First, FirstOrDefault
- Last, LastOrDefault
- Contains, Any, Count
- Sum, Min, Max

#### Todo

- Range
- Concat
- Distinct, Union, Intersect