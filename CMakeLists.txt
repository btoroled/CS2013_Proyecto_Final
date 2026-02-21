cmake_minimum_required(VERSION 3.14)
project(StreamingPlatform CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimización
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

# Threads (para std::async)
find_package(Threads REQUIRED)

include_directories(include)

add_executable(streaming src/main.cpp)
target_link_libraries(streaming Threads::Threads)

# Build type default
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
