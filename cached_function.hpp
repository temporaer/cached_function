/**
 *
 * Published under three-clause BSD license.
 *
 * Copyright 2014 Hannes Schulz <schulz@ais.uni-bonn.de>
 * 
 * Implements an almost transparent disk cache for C++ functions.
 * Depends heavily on C++11 features, such as variadic templates.
 * 
 * Another (optional) dependency is boost.log, which is contained 
 * in boost versions * >=1.55.
 * 
 * All function arguments must be hashable, side effects are not considered.
 *
 * Usage example below the header code.
 * 
 * Compile using:
 * $ g++ -DBOOST_ALL_DYN_LINK -DCFTEST -std=c++11 -x c++ cached_function.hpp -lboost_system -lboost_filesystem -lboost_serialization -pthread -lboost_log
 * and run with
 * $ ./a.out 42
 */
#ifndef __CACHED_FUNCTION_HPP__
#     define __CACHED_FUNCTION_HPP__
#include <fstream>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/functional/hash.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>

#define CACHED(cache, func, ...) cache(#func, func, __VA_ARGS__)

namespace fscache{
    namespace fs = boost::filesystem;
    template<typename... Args> inline void pass(Args&&...) {}
    struct function_cache{
        fs::path m_path;
        function_cache(std::string path = fs::current_path().string())
        :m_path(fs::path(path) / "cache"){
            fs::create_directories(m_path);
        }

        template<typename Func, typename... Params>
            auto operator()(Func f, Params... params) -> decltype(f(params...)) {
                return (*this)("anonymous", f, params...);
            }
        template<typename Func, typename... Params>
            auto operator()(std::string descr, Func f, Params... params) -> decltype(f(params...)) {
                typedef decltype(f(params...)) retval_t;
                std::size_t seed = 0;
                //pass(boost::hash_combine(seed, params)...);
                boost::hash_combine(seed, params...);
                std::string fn = descr + "-" + boost::lexical_cast<std::string>(seed);
                fn = (m_path / fn).string();
                if(fs::exists(fn)){
                    std::ifstream ifs(fn);
                    boost::archive::text_iarchive ia(ifs);
                    retval_t ret;
                    ia >> ret;
                    BOOST_LOG_TRIVIAL(info) << "Cached access from file "<<fn;
                    return ret;
                }
                retval_t ret = f(params...);
                BOOST_LOG_TRIVIAL(info) << "Non-cached access, file "<<fn;
                std::ofstream ofs(fn);
                boost::archive::text_oarchive oa(ofs);
                oa << ret;
                return ret;
            }
    };

}
#endif /* __CACHED_FUNCTION_HPP__ */


#ifdef CFTEST
#include <iostream>

long fib(long i){
    if(i==0)
        return 0;
    if(i==1)
        return 1;
    return fib(i-1) + fib(i-2);
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
    std::cout << c(fib, atoi(argv[1])) << std::endl;
    std::cout << c(fib, atoi(argv[1])) << std::endl;
    return 0;
}
#endif