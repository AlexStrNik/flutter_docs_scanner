cmake_minimum_required(VERSION 3.10)

include_directories(../include)

add_library(lib_opencv SHARED IMPORTED)
set_target_properties(lib_opencv PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libopencv_java4.so)

set(SOURCES_DIR "../ios/Classes")
set(SOURCES
    ${SOURCES_DIR}/flutter_docs_scanner.cpp
    ${SOURCES_DIR}/utils.hpp
)

add_library(flutter_docs_scanner SHARED $SOURCES)
set_target_properties(flutter_docs_scanner PROPERTIES
  PUBLIC_HEADER flutter_docs_scanner.h
  OUTPUT_NAME "flutter_docs_scanner"
)

target_link_libraries(flutter_docs_scanner lib_opencv)

