cmake_minimum_required(VERSION 3.10)

project(ArgParser VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED 20)

add_library(ArgParser STATIC 
    src/ArgParser.cpp
)

target_include_directories(ArgParser
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(ArgParser PUBLIC cxx_std_20)

install(TARGETS ArgParser
    EXPORT ArgParserTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

install(EXPORT ArgParserTargets
    FILE ArgParserConfig.cmake
    NAMESPACE ArgParser::
    DESTINATION lib/cmake/ArgParser
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ArgParserConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ArgParserConfigVersion.cmake"
    DESTINATION lib/cmake/ArgParser
)