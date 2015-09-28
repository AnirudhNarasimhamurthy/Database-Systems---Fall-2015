cmake_minimum_required(VERSION 3.3)
project(Project_2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    proj2-assign/include/BMTester.h
    proj2-assign/include/da_types.h
    proj2-assign/include/db.h
    proj2-assign/include/hfpage.h
    proj2-assign/include/index.h
    proj2-assign/include/minirel.h
    proj2-assign/include/new_error.h
    proj2-assign/include/page.h
    proj2-assign/include/system_defs.h
    proj2-assign/include/test_driver.h
    proj2-assign/src/BMTester.C
    proj2-assign/src/buf.C
    proj2-assign/src/buf.h
    proj2-assign/src/db.C
    proj2-assign/src/main.C
    proj2-assign/src/new_error.C
    proj2-assign/src/page.C
    proj2-assign/src/system_defs.C
    proj2-assign/src/test_driver.C)

add_executable(Project_2 ${SOURCE_FILES})