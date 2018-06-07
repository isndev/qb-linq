# Linq

Fast and Simplified C#.Net Linq for C++14 and above !

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
  #include <iostream>
  #include <vector>
  // only need this !
  #include "linq/linq.h"
  
  int main() {
    // my container 
    std::vector<int> container = { 1, 2, 3, 4, 5, 1, 2, 3, 4, 5 };
    
    // create enumerable
    auto enu = linq::make_enumerable(container);
    // pick only value > 3
    auto filter1 = enu.Where([](const auto &value) {
        return value > 3;
    });
    for (auto v : filter1) {
      std::cout << v;
    }
    // output: 4545
    
    // let's cumulate filters from filter1
    auto filter2 = filter1.Skip(1).Take(2);
    std::cout << filter2.First() << ":" << filter2.Last();
    // output: 5:4
    
    // make complex filter and edit value in container
    auto value_to_edit = 5;
    enu.Where([value_to_edit](const auto &value){ return value == value_to_edit; }) // where value = captured variable
       .Take(1) // pick only one
       .Each([](auto &value) { value = 100; }); // set it to 100
    // edited only the first encountred value in vector
    std::cout << vec[4] << ":" << enu.Last();
    // output: 100:5
    
    // All filters are lazy, no overhead on creation, reusable if proxy container lives
    // Stop naive run of your data, use linq for C++!
    
    return 0;
  }
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
