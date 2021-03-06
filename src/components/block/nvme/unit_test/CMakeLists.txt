cmake_minimum_required (VERSION 3.5.1 FATAL_ERROR)


project(blocknvme-tests CXX)

include_directories(${CMAKE_INSTALL_PREFIX}/include)
link_directories(${CMAKE_INSTALL_PREFIX}/lib)
link_directories(/usr/local/lib64)

add_definitions(${GCC_COVERAGE_COMPILE_FLAGS} -DCONFIG_DEBUG)

add_executable(blocknvme-test1 test1.cpp)
target_link_libraries(blocknvme-test1 ${ASAN_LIB} common comanche-core comanche-blknvme numa gtest pthread dl )
target_compile_features(blocknvme-test1 PRIVATE cxx_range_for)

add_executable(blocknvme-test2 test2.cpp)
target_link_libraries(blocknvme-test2 ${ASAN_LIB} common comanche-core comanche-blknvme numa gtest pthread dl )
target_compile_features(blocknvme-test2 PRIVATE cxx_range_for)

add_executable(blocknvme-test3 test3.cpp)
target_link_libraries(blocknvme-test3 ${ASAN_LIB} common comanche-core comanche-blknvme numa gtest pthread dl )
target_compile_features(blocknvme-test3 PRIVATE cxx_range_for)

add_executable(test-throughput test-throughput.cpp)
target_link_libraries(test-throughput ${ASAN_LIB} common comanche-core comanche-blknvme numa gtest pthread dl )
target_compile_features(test-throughput PRIVATE cxx_range_for)

add_test(unit_test blocknvme-test)
