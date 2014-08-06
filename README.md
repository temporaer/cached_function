Convenient Disk Cache/Memoization for C++
=========================================

Synopsis
--------

```c++
#include "memoization.hpp"

int fib(int) { ... }
memoization::disk c("cache_path");

int result1 = c("fib", fib, 3);

auto fib2 = memoization::make_memoized(c, "fib", fib);
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
name exists, the return value is loaded from disk. Otherwise the function is
executed.  As long as you put unique names for your functions, this should be
pretty safe.

The memory-version does not serialize to disk, it relies on copying.


Assumptions
-----------
For in-memory cache:

1. All function arguments must be hashable
2. The function does not have side effects

For the disk cache, additionally:

3. Returned object must be serializable
4. Returned object must be default constructible

For registry to work (in recursive functions, see below), the assumption is
that the function pointers are unique. This is only guaranteed for free
functions, so use with care.


Recursive Functions
-------------------

Recursive functions cannot be memoized without modifying them.  The
modification is made easier by storing references to the cache in a global
variable:

```c++
using namespace memoization;
long fib(long i){
    if(i<2) return i;
    return memoized<memory>(fib, i-1)
         + memoized<memory>(fib, i-2);
}
memory cache;
auto mfib = make_memoized(cache, "fib", fib);
long res = mfib(100);
```


Dependencies
------------

Depends heavily on C++11 features (auto, decltype, rvalue references,
variadic templates), and boost for hashing, serialization, and `any`.

Another (optional) dependency is boost.log, which is contained
in boost versions >=1.55.


License
-------
 
Published under three-clause BSD license.

Copyright 2014 Hannes Schulz <schulz@ais.uni-bonn.de>
