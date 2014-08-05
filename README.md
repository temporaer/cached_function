Convenient Disk Cache/Memoization for C++
=========================================

Synopsis
--------

```c++
#include "cached_function.hpp"

int fib(int) { ... }
fscache::cache c("cache_path");

int result1 = c("fib", fib, 3);

auto fib2 = decorator::make_memoized(c, "fib", fib);
int result2 = fib2(3);

assert(result1 == result2);
```

For more examples see the [test file](test_cache.cpp).

Features
--------
- Arbitrary number of arguments 
- Arbitrary return types
- If novel arguments are provided, the function is recomputed.

This works as follows: The arguments are hashed. If a file with the hash in its
name exists, the return value is loaded. Otherwise the function is executed.
As long as you put unique names for your functions, this should be pretty safe.


Assumptions
-----------
1. All function arguments must be hashable, side effects are not considered.
2. Returned object must be serializable
3. Returned object must be default constructible

Dependencies
------------

Depends heavily on C++11 features (auto, decltype, rvalue references,
variadic templates), and boost for hashing, serialization.

Another (optional) dependency is boost.log, which is contained
in boost versions * >=1.55.


License
-------
 
Published under three-clause BSD license.
Copyright 2014 Hannes Schulz <schulz@ais.uni-bonn.de>
