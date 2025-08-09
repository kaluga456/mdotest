cmake_minimum_required(VERSION 3.16)

project(mdotest)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

#platform check
if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    set(TARGET_HEADER "msvc_build.h")
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(TARGET_HEADER "gcc_build.h")
else()
    message(FATAL_ERROR "C compiler not supported: ${CMAKE_C_COMPILER_ID}")
endif()

#platform specific headers
configure_file(main.h.in "${CMAKE_CURRENT_SOURCE_DIR}/main.h")

set(PROJECT_SOURCES main.cpp)
	
add_executable(mdotest ${PROJECT_SOURCES})
