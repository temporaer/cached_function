/**
 * Implements an almost transparent disk cache/memoization for C++ functions.
 *
 * Published under three-clause BSD license.
 * Copyright 2014 Hannes Schulz <schulz@ais.uni-bonn.de>
 */
#ifndef __MEMOIZATION_HPP_295387__
#     define __MEMOIZATION_HPP_295387__
#include <map>
#include <fstream>
#include <utility>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/functional/hash.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/any.hpp>
#include <boost/log/trivial.hpp>

#define CACHED(cache, func, ...) cache(#func, func, __VA_ARGS__)

namespace memoization{
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
    struct disk{
        fs::path m_path;
        disk(std::string path = fs::current_path().string())
        :m_path(fs::path(path) / "cache"){
            fs::create_directories(m_path);
        }

        template<typename Func, typename... Params>
            auto operator()(const Func& f, Params&&... params) -> decltype(f(params...))const{
                return (*this)("anonymous", f, std::forward<Params>(params)...);
            }
        template<typename Func, typename... Params>
            auto operator()(const std::string& descr, const Func& f, Params&&... params) -> decltype(f(params...))const{
                std::size_t seed = detail::hash_combine(0, descr, params...);
                return (*this)(descr, seed, f, std::forward<Params>(params)...);
            }
        template<typename Func, typename... Params>
            auto operator()(const std::string& descr, std::size_t seed, const Func& f, Params&&... params) -> decltype(f(params...))const{
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

    struct memory{
        mutable std::map<std::size_t, boost::any> m_data;

        template<typename Func, typename... Params>
            auto operator()(const Func& f, Params&&... params) -> decltype(f(params...)) const {
                return (*this)("anonymous", f, std::forward<Params>(params)...);
            }
        template<typename Func, typename... Params>
            auto operator()(std::string descr, const Func& f, Params&&... params) -> decltype(f(params...)) const {
                std::size_t seed = detail::hash_combine(0, descr, params...);
                return (*this)(seed, f, std::forward<Params>(params)...);
            }
        template<typename Func, typename... Params>
            auto operator()(const std::string& descr, std::size_t seed, const Func& f, Params&&... params) -> decltype(f(params...)) const {
                boost::hash_combine(seed, descr);
                return (*this)(seed, f, std::forward<Params>(params)...);
            }
        template<typename Func, typename... Params>
            auto operator()(std::size_t seed, const Func& f, Params&&... params) -> decltype(f(params...)) const {
                typedef decltype(f(params...)) retval_t;
                auto it = m_data.find(seed);
                if(it != m_data.end()){
                    BOOST_LOG_TRIVIAL(info) << "Cached access from memory";
                    return boost::any_cast<retval_t>(it->second);
                }
                retval_t ret = f(std::forward<Params>(params)...);
                BOOST_LOG_TRIVIAL(info) << "Non-cached access";
                m_data[seed] = ret;
                return ret;
            }
    };



    template<typename Cache, typename Function>
    struct memoize{
        const Function& m_func; // we require copying the function object here.
        std::string m_id;
        Cache& m_fc;
        memoize(Cache& fc, std::string id, const Function& f)
            :m_func(f), m_id(id), m_fc(fc){}
        template<typename... Params>
        auto operator()(Params&&... args) 
                -> decltype(std::bind(m_func, args...)()){
            return m_fc(m_id, m_func, std::forward<Params>(args)...);
        }
    };
    template<class Cache, class Function>
    struct registry{
        static std::map<Function, std::pair<std::string, Cache*> > data;
    };
    template<class Cache, class Function>
    std::map<Function, std::pair<std::string, Cache*> >
    registry<Cache, Function>::data;
        
    template<typename Cache, typename Function>
    memoize<Cache, Function>
    make_memoized(Cache& fc, const std::string& id, Function f){
        typedef registry<Cache,Function> reg_t;
        auto it = reg_t::data.find(f);
        if(it == reg_t::data.end()){
            BOOST_LOG_TRIVIAL(info) << "registering " << id << " in registry";
            reg_t::data[f] = std::make_pair(id, &fc);
        }
        return memoize<Cache, Function>(fc, id, f);
    }

    template<typename Cache, typename Function, typename...Args>
    auto
    memoized(Function f, Args&&... args) -> decltype(std::bind(f, args...)()){
        typedef registry<Cache,Function> reg_t;
        auto it = reg_t::data.find(f);
        if(it == reg_t::data.end())
            throw std::runtime_error("memoize function is not registered with a cache");
        std::string id;
        Cache* fc;
        std::tie(id,fc) = it->second;
        return memoize<Cache, Function>(*fc, id, f)(std::forward<Args>(args)...);
    }

}
#endif /* __MEMOIZATION_HPP_295387__ */
