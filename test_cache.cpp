#include <iostream>
#include <boost/serialization/vector.hpp>
#include "memoization.hpp"

long fib(long i){
    if(i==0)
        return 0;
    if(i==1)
        return 1;
    return fib(i-1) + fib(i-2);
}

std::vector<int> times(const std::vector<int>& v, int factor){
    std::vector<int> v2 = v;
    for(int& i : v2)
        i *= factor;
    return v2;
}

void test_disk_cache(int i){
    memoization::disk c;
    // name cache same as function name
    std::cout << CACHED(c, fib, i) << std::endl;
    std::cout << CACHED(c, fib, i) << std::endl;

    // lambda function -- this should be pretty safe, but results in awkward
    // file names.
    std::cout << CACHED(c, [](int i){return fib(i+2);}, i) << std::endl;
    std::cout << CACHED(c, [](int i){return fib(i+2);}, i) << std::endl;

    // convenient, but dangerous: only looks at arguments, only use for
    // functions with very different signature
    std::cout << c(fib, i+1) << std::endl;
    std::cout << c(fib, i+1) << std::endl;

    // unhashable arg, use own, arbitrary hash and clean up cache yourself
    std::cout << c("fib", 28725, fib, i+2) << std::endl;
    std::cout << c("fib", 28725, fib, i+2) << std::endl;

    // memoize: generate a new function that can be called
    // w/o reference to the cache
    auto fib2 = memoization::make_memoized(c, "fib2", [](int i){return fib(i+2);});
    std::cout << fib2(i) << std::endl;
    std::cout << fib2(i) << std::endl;

    // more complex argument types also work, as long as they are
    // default-constructible, hashable and serializable:
    std::vector<int> v(10000, i), v2, v3;
    v2 = CACHED(c, times, v, 5);
    v3 = CACHED(c, times, v, 5);
    assert(v2 == v3);
}

void test_mem_cache(int i){
    memoization::memory c;
    // name cache same as function name
    std::cout << CACHED(c, fib, i) << std::endl;
    std::cout << CACHED(c, fib, i) << std::endl;

    // lambda function -- this should be pretty safe, but results in awkward
    // file names.
    std::cout << CACHED(c, [](int i){return fib(i+2);}, i) << std::endl;
    std::cout << CACHED(c, [](int i){return fib(i+2);}, i) << std::endl;

    // convenient, but dangerous: only looks at arguments, only use for
    // functions with very different signature
    std::cout << c(fib, i+1) << std::endl;
    std::cout << c(fib, i+1) << std::endl;

    // unhashable arg, use own, arbitrary hash and clean up cache yourself
    std::cout << c(28725, fib, i+2) << std::endl;
    std::cout << c(28725, fib, i+2) << std::endl;

    // memoize: generate a new function that can be called
    // w/o reference to the cache
    auto fib2 = memoization::make_memoized(c, "fib2", [](int i){return fib(i+2);});
    std::cout << fib2(i) << std::endl;
    std::cout << fib2(i) << std::endl;

    // more complex argument types also work, as long as they are
    // default-constructible, hashable and serializable:
    std::vector<int> v(10000, i), v2, v3;
    v2 = CACHED(c, times, v, 5);
    v3 = CACHED(c, times, v, 5);
    assert(v2 == v3);
}

int
main(int argc, char **argv)
{
    if(argc != 2){
        std::cout << "Usage: " << argv[0] << " N" << std::endl;
        std::cout << " where N is the index of the fibonacci number to compute" << std::endl;
        exit(1);
    }
    test_disk_cache(atoi(argv[1]));
    test_mem_cache(atoi(argv[1]));

    return 0;
}
