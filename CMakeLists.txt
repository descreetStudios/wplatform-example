cmake_minimum_required(VERSION 3.24)
project(wplatform_example VERSION 0.1.0 LANGUAGES CXX)

add_executable(wplatform_example main.cpp)

set(WPLATFORM_SOURCE_DIR "" CACHE PATH "Local wplatform source directory")

if(WPLATFORM_SOURCE_DIR)
    add_subdirectory("${WPLATFORM_SOURCE_DIR}" wplatform)
else()
    include(FetchContent)
    FetchContent_Declare(wplatform
        GIT_REPOSITORY https://github.com/descreetStudios/wplatform.git
        GIT_TAG main
    )
    FetchContent_MakeAvailable(wplatform)
endif()

target_link_libraries(wplatform_example PRIVATE
    wplatform::wplatform
)
