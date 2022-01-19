cmake_minimum_required(VERSION 2.8)
project(eIODP)

# cmake设置
set(CMAKE_BUILD_TYPE "Debug")
#SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -W -g -ggdb")
#SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -W")
SET(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-pthread")


# 设置执行文件输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# 设置库输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)


message("++++++++++++++++++++Start Build++++++++++++++++++++++++++")

# 头文件搜索目录
include_directories(${PROJECT_SOURCE_DIR}/src/eiodp/include)

# 链接库搜索目录
link_directories(${PROJECT_BINARY_DIR}/lib /usr/local/lib /usr/lib)


#设置链接库
add_library(${PROJECT_NAME} SHARED
        src/eiodp/eiodp.c 
        src/eiodp/ring.c 
        src/udpio/udpio.c
)

#target_link_libraries(${PROJECT_NAME}
#    cryptopp
#)


# 可执行文件

add_executable(test_rwaddr test/test_rwaddr.c)
target_link_libraries(test_rwaddr ${PROJECT_NAME})

add_executable(test_showregfunc test/test_showregfunc.c)
target_link_libraries(test_showregfunc ${PROJECT_NAME})

add_executable(test_func test/test_func.c)
target_link_libraries(test_func ${PROJECT_NAME})

add_executable(test_nos test/test_nos.c)
target_link_libraries(test_nos ${PROJECT_NAME})
