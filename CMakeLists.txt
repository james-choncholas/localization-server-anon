cmake_minimum_required(VERSION 3.12)
project(SecureLocal LANGUAGES CXX)

# YouCompleteMe
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )
execute_process(COMMAND ln -s ln -s build/compile_commands.json ./ )

if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
    message(FATAL_ERROR "requires at least g++-8")
endif()

#set (CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb -fno-omit-frame-pointer")

# Set build type to `Release` if non was specified:
# (cf. https://gitlab.kitware.com/cmake/community/wikis/FAQ#how-can-i-change-the-default-build-mode-and-see-it-reflected-in-the-gui)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# Write built executables and libraries to bin/ and lib/, respectively.
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(extern/ABY)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)



# To used an opencv version installed in build
# You might need to do this first???
#set(OpenCV_DIR ${CMAKE_CURRENT_BINARY_DIR})
#set(BUILD_SHARED_LIBS OFF)
#add_subdirectory(extern/opencv)
#set(INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/")

#find_package(OpenCV REQUIRED)
#add_library(opencv INTERFACE)
#include_directories(${OpenCV_INCLUDE_DIRS})

# To use the version installed on the system
#find_package(opencv4 QUIET)

execute_process(COMMAND mkdir build
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/opencv/")
execute_process(COMMAND cmake ..
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/opencv/build")
execute_process(COMMAND make
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/opencv/build")
execute_process(COMMAND sudo make install
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/opencv/build")
set(OpenCV_DIR "")
find_package(OpenCV REQUIRED)
#include_directories(${OpenCV_INCLUDE_DIRS})
message(STATUS "cv libs: ${OpenCV_LIBS}")
message(STATUS "include_path: ${OpenCV_INCLUDE_DIRS}")




foreach(empdir tool ot sh2pc)
    #execute_process(COMMAND make clean
    #    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/emp-${empdir}")
    execute_process(COMMAND cmake -DENABLE_FLOAT=On -DTHREADING=On .
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/emp-${empdir}")
    execute_process(COMMAND make -j8
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/emp-${empdir}")
    execute_process(COMMAND sudo make install
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/emp-${empdir}")
    find_package(emp-tool REQUIRED)
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/extern/emp-${empdir}")
endforeach()
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(emp-tool REQUIRED)
find_package(emp-ot REQUIRED)
find_package(emp-sh2pc REQUIRED)
#find_package(GMP REQUIRED)
include(${CMAKE_CURRENT_SOURCE_DIR}/extern/emp-sh2pc/cmake/emp-sh2pc-config.cmake)

# turn on threading for this code b/c it's turned on for emp
ADD_DEFINITIONS(-DTHREADING)



#find_package(RELIC::relic REQUIRED)
#add_subdirectory(extern/relic)

#find_package(RELIC QUIET)
#find_package(RELIC::relic REQUIRED)
#add_library(RELIC::relic STATIC IMPORTED)

#find_package(GMP REQUIRED)
#find_package(Threads REQUIRED)
#find_package(Boost 1.66.0 REQUIRED COMPONENTS thread system)

add_subdirectory(src/)
add_subdirectory(test/)
