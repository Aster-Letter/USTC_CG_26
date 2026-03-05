cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

# VS 2026 (18.x) support depends on the CMake generator version.
# Generator selection happens before this file is parsed, so we only provide
# an explicit diagnostic here to avoid confusing downstream errors.
if(CMAKE_GENERATOR MATCHES "^Visual Studio" AND CMAKE_VERSION VERSION_LESS "3.30")
    message(FATAL_ERROR
        "Detected Visual Studio generator with CMake ${CMAKE_VERSION}. "
        "Your Visual Studio is 2026 (18.x), which is not supported by older "
        "CMake Visual Studio generators. Please upgrade CMake to a newer "
        "version that supports VS 2026, or use Ninja/NMake as generator.")
endif()

project(CppPractices VERSION 1.0)
message(STATUS "[Project] ${PROJECT_NAME} ${PROJECT_VERSION}")

include(cmake/helper.cmake)

set(OUTPUT_PROP   
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_SOURCE_DIR}/bin"
)

add_subdirectory(src)