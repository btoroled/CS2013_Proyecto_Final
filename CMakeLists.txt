cmake_minimum_required(VERSION 4.1)
project(1)

set(CMAKE_CXX_STANDARD 20)

add_executable(1 main.cpp
        src/core/Movie.cpp
        include/core/Movie.h
        src/core/User.cpp
        include/core/User.h
        src/core/UserStore.cpp
        include/core/UserStore.h
        src/index/Trie.cpp
        include/index/Trie.h
        src/index/WordIndex.cpp
        include/index/WordIndex.h
        src/index/NgramIndex.cpp
        include/index/NgramIndex.h
        src/core/StreamingPlatform.cpp
        include/core/StreamingPlatform.h
        src/ui/UI.cpp
        include/ui/UI.h
        src/text/TextUtils.cpp
        include/text/TextUtils.h)
