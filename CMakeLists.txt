cmake_minimum_required(VERSION 3.5)
project(eIODP)
#project(eIODP LANGUAGES CXX)
# cmake设置
set(CMAKE_BUILD_TYPE "Debug")
#SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -W -g -ggdb")
#SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -W")
SET(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-pthread")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")  #设置c++的编译选项

#QT设置
#set(CMAKE_INCLUDE_CURRENT_DIR ON)
#set(CMAKE_AUTOUIC ON)
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)


#find_package(Qt5Core)


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
if (WIN32)
link_libraries(wsock32)
endif()

add_library(${PROJECT_NAME} STATIC
        src/eiodp/eiodp.c 
        src/eiodp/ring.c
        src/udpio/udpio.c 
)







# 可执行文件
if (UNIX)
    add_executable(test_rwaddr test/test_rwaddr.c)
    target_link_libraries(test_rwaddr ${PROJECT_NAME})

    add_executable(test_showregfunc test/test_showregfunc.c)
    target_link_libraries(test_showregfunc ${PROJECT_NAME})

    add_executable(test_func test/test_func.c)
    target_link_libraries(test_func ${PROJECT_NAME})

    add_executable(test_nos test/test_nos.c)
    target_link_libraries(test_nos ${PROJECT_NAME})
elseif(WIN32)
    add_executable(test_rwaddr test/test_rwaddr.c)
    target_link_libraries(test_rwaddr ${PROJECT_NAME})

    add_executable(test_showregfunc test/test_showregfunc.c)
    target_link_libraries(test_showregfunc ${PROJECT_NAME})

    add_executable(test_func test/test_func.c)
    target_link_libraries(test_func ${PROJECT_NAME})

    add_executable(test_nos test/test_nos.c)
    target_link_libraries(test_nos ${PROJECT_NAME})

    add_executable(test_udpio test/test_udpio.c)
    target_link_libraries(test_udpio ${PROJECT_NAME})
    ##add_executable(test_qtfunc test/test_qtfunc.c )
    ##target_link_libraries(test_qtfunc ${PROJECT_NAME})
endif()
