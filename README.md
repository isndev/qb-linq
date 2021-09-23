# Linq

Fast and Simplified C#.Net Linq for C++14 and above !

### Requirements
- CMake (optional)
```cmake
    add_subdirectory(linq_directory)
    # ...
    # linq is an INTERFACE library (only headers)
    # link with it will just give access to linq include directory
    target_link_libraries(your_project linq)
```
- Add to your code
```cpp
    // only one file to include
    // using cmake 
    #include <linq/linq.h>
    // without cmake
    #include "path_to_linq_include_dir/linq.h"
```
- min C++14
- works on Linux/Windows/Mac

### Usage

#### Get Started
- Create Enumerable
```cpp
    // create enumerable from any iterable container
    // vector, map, etc...
    std::vector<int> data{0, 1, 2, 3, 4, 5};
    auto enu = linq::make_enumerable(data);
    // or from iterable range
    auto enu_range = linq::make_enumerable(data.begin() + 1, data.end());
    // of create enumerable from existing enumerable
    auto enu_filter = enu.Where([](auto &val){ return val < 3; });
    // you are now ready to use them !
```
- Run on Enumerable
```cpp
    // let's run on data
    // 1st method: using Each method
    enu.Each([](auto &val){
        // do something...
    });
    // 2nd method: simple for
    for (auto &val : enu) {
        // do something...
    }
    // you are now ready to combine enumerable
    // with all supported operations
```
#### Supported operations

- Each
```cpp
    std::vector<int> data{0, 1, 2, 3, 4, 5};

    linq::make_enumerable(data)
        .Each([](auto &val)){
            std::cout << val << " ";
        });
    // output: 0 1 2 3 4 5
```
- Select
```cpp
    struct User {
        int id;
        int groupId;
        // other members...
    };
    std::vector<User> data{{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}};
    linq::make_enumerable(data)
        .Select([](auto &user) { return user.id; })
        .Each([](auto &id) {
            std::cout << id << " ";
        });
    // output: 0 1 2 3 4 5 
```
- SelectMany
```cpp
    struct User {
        int id;
        int groupId;
        // ...
    };
    std::vector<User> data{{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}};
    linq::make_enumerable(data)
        .Select([](auto &user) { return user.id; },
                [](auto &user) { return user.groupId; })
        .Each([](auto &tuple) {
            std::cout << std::get<0>(tuple) << ":" << std::get<1>(tuple) << " ";
        });
    // output: 0:1 1:2 2:3 3:4 4:5 5:6 
```
- Where
```cpp
    std::vector<int> data{0, 1, 2, 3, 4, 5};
    
    linq::make_enumerable(data)
        .Where([](auto &val){ return val > 3; })
        .Each([](auto &val)){
            std::cout << val << " ";
        });
    // output: 4 5
```
- OrderBy
```cpp
    struct User {
        int id;
        int groupId;
        // other members...
    };

    std::vector<User> data{{0, 1}, {1, 1}, {2, 1}, {3, 2}, {4, 2}, {5, 2}};
    
    linq::make_enumerable(data)
        .OrderBy(linq::asc([](auto &user){ return user.groupId; }), // order by groupId asc
                 linq::desc([](auto &user){ return user.id; })) // then by id desc
        .Each([](auto &user) {
            std::cout << user.groupId << ":" << user.id << " ";
        });
    // output: 1:2 1:1 1:0 2:5 2:4 2:3 
```
- GroupBy
```cpp
    struct User {
        int id;
        int groupId;
        // other members...
    };

    std::vector<User> data{{0, 1}, {1, 1}, {2, 1}, {3, 2}, {4, 2}, {5, 2}};
    
    linq::make_enumerable(data)
        .GroupBy([](auto &user){ return user.groupId; }) // group by groupId
        .Each([](auto &pair) {
            // pair.first is groupId
            // pair.second is user reference
            std::cout << pair.first << ":" << pair.second.id << " ";
        });
    // possible output: 1:0 1:1 1:2 2:3 2:4 2:5 
```
- Skip
```cpp
    std::vector<int> data{0, 1, 2, 3, 4, 5};
    
    linq::make_enumerable(data)
        .Skip(3)
        .Each([](auto &val)){
            std::cout << val << " ";
        });
    // output: 3 4 5
```
- SkipWhile
```cpp
    std::vector<int> data{0, 1, 2, 3, 4, 5};
    
    linq::make_enumerable(data)
        .SkipWhile([](auto &val){ return val < 3; })
        .Each([](auto &val)){
            std::cout << val << " ";
        });
    // output: 3 4 5
```
- Take
```cpp
    std::vector<int> data{0, 1, 2, 3, 4, 5};
    
    linq::make_enumerable(data)
        .Take(3)
        .Each([](auto &val)){
            std::cout << val << " ";
        });
    // output: 0 1 2
```
- TakeWhile
```cpp
    std::vector<int> data{0, 1, 2, 3, 4, 5};
    
    linq::make_enumerable(data)
        .TakeWhile([](auto &val){ return val < 3; })
        .Each([](auto &val)){
            std::cout << val << " ";
        });
    // output: 0 1 2
```
- First, Last
```cpp
    std::vector<int> data{0, 1, 2, 3, 4, 5};
    
    auto enu = linq::make_enumerable(data);
    std::cout << enu.First() << ":" << enu.Last(); // /!\ crash if empty
    // output: 0:5
```
- FirstOrDefault, LastOrDefault
```cpp
    std::vector<int> data{};

    auto enu = linq::make_enumerable(data);
    std::cout << enu.FirstOrDefault() << ":" << enu.LastOrDefault();
    // output: 0:0 (default int)
```
- Contains, Any, Count
```cpp
    std::vector<int> data{0, 1, 2, 3 , 4, 5};

    auto enu = linq::make_enumerable(data);
    std::cout << enu.Contains(6) << std::endl
              << enu.Any() << std::endl
              << enu.Count() << std::endl;
    
    // output: false
    //         true
    //         6
```
- Sum, Min, Max
```cpp
    std::vector<int> data{0, 1, 2, 3 , 4, 5};

    auto enu = linq::make_enumerable(data);
    std::cout << enu.Sum() << std::endl
              << enu.Min() << std::endl
              << enu.Max() << std::endl;
    
    // output: 15
    //         0
    //         5
```

- Advanced Example
```cpp
  #include <iostream>
  #include <vector>
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
    // Stop naive run on your data, use linq for C++!
    
    return 0;
  }
```

### Build Tests/Benchmarks

```bash
  $> cmake -Bbuild -DQB_BUILD_TEST=ON -H.
  $> cd build && make
  $> ./qb-linq-overhead && ./qb-linq-benchmark
```

#### Todo

- Concat
- Distinct, Union, Intersect

