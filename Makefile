all: test_cache
test_cache: test_cache.cpp
	g++ -DBOOST_ALL_DYN_LINK -DCFTEST -std=c++11 test_cache.cpp -lboost_system -lboost_filesystem -lboost_serialization -pthread -lboost_log -o test_cache
run: test_cache
	./test_cache 42
