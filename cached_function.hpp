/**
 * Implements an almost transparent disk cache/memoization for C++ functions.
 *
 * Published under three-clause BSD license.
 * Copyright 2014 Hannes Schulz <schulz@ais.uni-bonn.de>
 *
 * Synopsis:
 *
 * @code
 * int fib(int) { ... }
 * fscache::function_cache c("cache_path");
 *
 * int result1 = c("fib", fib, 3);
 *
 * auto fib2 = make_memoized(c, "fib", fib);
 * int result2 = fib2(3);
 *
 * assert(result1 == result2);
 * @endcode
 *
 * More usage example are below the header code.
 *
 * Depends heavily on C++11 features (auto, decltype, rvalue references,
 * variadic templates)
 *
 * Another (optional) dependency is boost.log, which is contained
 * in boost versions * >=1.55.
 *
 * Preconditions are:
 * - All function arguments must be hashable, side effects are not considered.
 * - Returned object must be serializable
 * - Returned object must be default constructible
 *
 * Compile using:
 * @code
 * $ g++ -DBOOST_ALL_DYN_LINK -DCFTEST -std=c++11 -x c++ cached_function.hpp -lboost_system -lboost_filesystem -lboost_serialization -pthread -lboost_log
 * @endcode
 * and run with
 * @code
 * $ ./a.out 42
 * @endcode
 */
#ifndef __CACHED_FUNCTION_HPP__
#     define __CACHED_FUNCTION_HPP__
#include <fstream>
#include <utility>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/functional/hash.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>

#define CACHED(cache, func, ...) cache(#func, func, __VA_ARGS__)
#define LOGSTARTSTOP(func, ...) decorator::make_logstartstop(#func, func)

namespace fscache{
    namespace fs = boost::filesystem;
    namespace detail{
        template <typename T>
            size_t hash_combine(std::size_t seed, const T& t) {
                boost::hash_combine(seed, t);
                return seed;
            }
        template <typename T, typename... Params>
            size_t hash_combine(std::size_t seed, const T& t, const Params&... params) {
                boost::hash_combine(seed, t);
                return hash_combine(seed, params...);
            }
    }
    struct function_cache{
        fs::path m_path;
        function_cache(std::string path = fs::current_path().string())
        :m_path(fs::path(path) / "cache"){
            fs::create_directories(m_path);
        }

        template<typename Func, typename... Params>
            auto operator()(Func f, Params&&... params) -> decltype(f(params...)) {
                return (*this)("anonymous", f, std::forward<Params>(params)...);
            }
        template<typename Func, typename... Params>
            auto operator()(std::string descr, Func f, Params&&... params) -> decltype(f(params...)) {
                std::size_t seed = detail::hash_combine(0, descr, params...);
                return (*this)(descr, seed, f, std::forward<Params>(params)...);
            }
        template<typename Func, typename... Params>
            auto operator()(std::string descr, std::size_t seed, Func f, Params&&... params) -> decltype(f(params...)) {
                typedef decltype(f(params...)) retval_t;
                std::string fn = descr + "-" + boost::lexical_cast<std::string>(seed);
                fn = (m_path / fn).string();
                if(fs::exists(fn)){
                    std::ifstream ifs(fn);
                    boost::archive::binary_iarchive ia(ifs);
                    retval_t ret;
                    ia >> ret;
                    BOOST_LOG_TRIVIAL(info) << "Cached access from file "<<fn;
                    return ret;
                }
                retval_t ret = f(std::forward<Params>(params)...);
                BOOST_LOG_TRIVIAL(info) << "Non-cached access, file "<<fn;
                std::ofstream ofs(fn);
                boost::archive::binary_oarchive oa(ofs);
                oa << ret;
                return ret;
            }
    };

}
namespace decorator{
    template<typename Function>
    struct memoize{
        Function m_func;
        std::string m_id;
        fscache::function_cache& m_fc;
        memoize(fscache::function_cache& fc, std::string id, Function f)
            :m_func(f), m_id(id), m_fc(fc){}
        template<typename... Params>
        auto operator()(Params&&... args)
                -> decltype(std::bind(m_func, args...)()){
            return m_fc(m_id, m_func, args...);
        }
    };
    template<typename Function>
    memoize<Function>
    make_memoized(fscache::function_cache& fc, std::string id, Function f){
        return memoize<Function>(fc, id, f);
    }

    template<typename Function>
    struct logstartstop{
        // logging works only on non-void m_func at the moment!
        Function m_func;
        std::string m_id;
        logstartstop(std::string id, Function f)
            :m_func(f), m_id(id){}
        template<typename... Params>
        auto operator()(Params&&... args)
                -> decltype(std::bind(m_func, args...)()){
            typedef decltype(std::bind(m_func, args...)()) ret_t;
            BOOST_LOG_TRIVIAL(info) << "BEGIN `"<<m_id<<"'";
            ret_t ret = std::bind(m_func, args...)();
            BOOST_LOG_TRIVIAL(info) << "END `"<<m_id<<"'";
            return ret;
        }
    };
    template<typename Function>
    logstartstop<Function>
    make_logstartstop(std::string id, Function f){
        return logstartstop<Function>(id, f);
    }
}
#endif /* __CACHED_FUNCTION_HPP__ */


#ifdef CFTEST
#include <iostream>
#include <boost/serialization/vector.hpp>

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
    fscache::function_cache c;
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
#endif
