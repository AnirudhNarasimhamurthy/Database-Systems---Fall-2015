cmake_minimum_required(VERSION 3.3)
project(proj5_assign)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    include/bitmap.h
    include/bt.h
    include/btfile.h
    include/btindex_page.h
    include/btleaf_page.h
    include/btreefilescan.h
    include/buf.h
    include/catalog.h
    include/da_types.h
    include/db.h
    include/ext_sys_defs.h
    include/heapfile.h
    include/hfpage.h
    include/index.h
    include/index_scan.h
    include/lru.h
    include/minirel.h
    include/mru.h
    include/new_error.h
    include/page.h
    include/scan.h
    include/SMJTester.h
    include/sort.h
    include/sorted_page.h
    include/system_defs.h
    include/test_driver.h
    include/tuple.h
    src/db.C
    src/main.C
    src/new_error.C
    src/page.C
    src/scan.C
    src/smjoin_main.C
    src/SMJTester.C
    src/sort_driver.C
    src/sorted_page.C
    src/sortMerge.C
    src/sortMerge.h
    src/system_defs.C
    src/test_driver.C)

add_executable(proj5_assign ${SOURCE_FILES})