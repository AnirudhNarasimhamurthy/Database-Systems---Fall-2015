cmake_minimum_required(VERSION 3.3)
project(HFPage)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    include/buf.h
    include/da_types.h
    include/db.h
    include/heapfile.h
    include/hfp_driver.h
    include/hfpage.h
    include/minirel.h
    include/new_error.h
    include/page.h
    include/scan.h
    include/system_defs.h
    include/test_driver.h
    src/db.C
    src/hfp_driver.C
    src/hfpage.C
    src/hfpage.h
    src/main.C
    src/new_error.C
    src/page.C
    src/system_defs.C
    src/test_driver.C)

add_executable(HFPage ${SOURCE_FILES})