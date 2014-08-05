Convenient Disk Cache/Memoization for C++
=========================================

Synopsis
--------

```c++
int fib(int) { ... }
fscache::function_cache c("cache_path");

int result1 = c("fib", fib, 3);

auto fib2 = make_memoized(c, "fib", fib);
int result2 = fib2(3);

assert(result1 == result2);
```

For more examples see the [test file](test_cache.cpp).

Dependencies
------------

Depends heavily on C++11 features (auto, decltype, rvalue references,
variadic templates)

Another (optional) dependency is boost.log, which is contained
in boost versions * >=1.55.

Preconditions are:
- All function arguments must be hashable, side effects are not considered.
- Returned object must be serializable
- Returned object must be default constructible

License
-------
 
Published under three-clause BSD license.
Copyright 2014 Hannes Schulz <schulz@ais.uni-bonn.de>
