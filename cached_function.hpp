/**
 * Implements an almost transparent disk cache/memoization for C++ functions.
 *
 * Published under three-clause BSD license.
 * Copyright 2014 Hannes Schulz <schulz@ais.uni-bonn.de>
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
    struct cache{
        fs::path m_path;
        cache(std::string path = fs::current_path().string())
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
        fscache::cache& m_fc;
        memoize(fscache::cache& fc, std::string id, Function f)
            :m_func(f), m_id(id), m_fc(fc){}
        template<typename... Params>
        auto operator()(Params&&... args)
                -> decltype(std::bind(m_func, args...)()){
            return m_fc(m_id, m_func, std::forward<Params>(args)...);
        }
    };
    template<typename Function>
    memoize<Function>
    make_memoized(fscache::cache& fc, std::string id, Function f){
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
            ret_t ret = m_func(std::forward<Params>(args)...);
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
