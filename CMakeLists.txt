cmake_minimum_required(VERSION 3.24)
project(wplatform_example VERSION 0.1.0 LANGUAGES CXX)

set(WPLATFORM_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../wplatform" CACHE PATH
    "Path to a local wplatform checkout; leave empty to fetch from GitHub")

include(FetchContent)
if(WPLATFORM_SOURCE_DIR AND EXISTS "${WPLATFORM_SOURCE_DIR}/CMakeLists.txt")
    FetchContent_Declare(wplatform SOURCE_DIR "${WPLATFORM_SOURCE_DIR}")
else()
    FetchContent_Declare(wplatform
        GIT_REPOSITORY https://github.com/descreetStudios/wplatform.git
        GIT_TAG main
    )
endif()
FetchContent_MakeAvailable(wplatform)

add_executable(wplatform_example main.cpp)
target_link_libraries(wplatform_example PRIVATE wplatform::wplatform)
