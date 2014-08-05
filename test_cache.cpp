#include <iostream>
#include <boost/serialization/vector.hpp>
#include "cached_function.hpp"

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

int
main(int argc, char **argv)
{
    fscache::cache c;
    if(argc != 2){
        std::cout << "Usage: " << argv[0] << " N" << std::endl;
        std::cout << " where N is the index of the fibonacci number to compute" << std::endl;
        exit(1);
    }
    // name cache same as function name
    std::cout << CACHED(c, fib, atoi(argv[1])) << std::endl;
    std::cout << CACHED(c, fib, atoi(argv[1])) << std::endl;

    // lambda function -- this should be pretty safe, but results in awkward
    // file names.
    std::cout << CACHED(c, [](int i){return fib(i+2);}, atoi(argv[1])) << std::endl;
    std::cout << CACHED(c, [](int i){return fib(i+2);}, atoi(argv[1])) << std::endl;

    // convenient, but dangerous: only looks at arguments, only use for
    // functions with very different signature
    std::cout << c(fib, atoi(argv[1])+1) << std::endl;
    std::cout << c(fib, atoi(argv[1])+1) << std::endl;

    // unhashable arg, use own, arbitrary hash and clean up cache yourself
    std::cout << c("fib", 28725, fib, atoi(argv[1])+2) << std::endl;
    std::cout << c("fib", 28725, fib, atoi(argv[1])+2) << std::endl;

    // memoize: generate a new function that can be called
    // w/o reference to the cache
    auto fib2 = decorator::make_memoized(c, "fib2", [](int i){return fib(i+2);});
    std::cout << fib2(atoi(argv[1]+3)) << std::endl;
    std::cout << fib2(atoi(argv[1]+3)) << std::endl;

    // more complex argument types also work, as long as they are
    // default-constructible, hashable and serializable:
    std::vector<int> v(10000, atoi(argv[1])), v2, v3;
    v2 = CACHED(c, times, v, 5);
    v3 = CACHED(c, times, v, 5);
    assert(v2 == v3);

    // logging (just another decorator for fun)
    auto fib3 = LOGSTARTSTOP([&](int i){return fib2(i+3);});
    std::cout << fib3(atoi(argv[1]+3)) << std::endl;

    return 0;
}
