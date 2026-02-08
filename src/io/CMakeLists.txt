set(SOURCES 
    Compressor.cpp
    encdec/Base64Encoder.cpp
)
set(LIBS)
set(HEADERS)

set(COMPILE_DEFINITIONS ${CSICS_COMPILE_DEFINITIONS})

if (CSICS_USE_ZSTD)
    list(APPEND SOURCES ZSTDCompressor.cpp)
    list(APPEND LIBS ${ZSTD_LIBRARIES})
    list(APPEND HEADERS ${ZSTD_INCLUDE_DIRS})
endif()

if (CSICS_USE_ZLIB)
    list(APPEND SOURCES ZLIBCompressor.cpp)
    list(APPEND LIBS ${ZLIB_LIBRARIES})
    list(APPEND HEADERS ${ZLIB_INCLUDE_DIRS})
endif()

add_subdirectory(net)
list(APPEND LIBS net)
add_library(io STATIC ${SOURCES})
target_compile_definitions(io PUBLIC ${COMPILE_DEFINITIONS})
target_compile_options(io PRIVATE ${CSICS_COMPILE_FLAGS})
target_link_options(io PRIVATE ${CMAKE_LINKER_FLAGS})
message(DEBUG "IO module sources: ${SOURCES}")
message(DEBUG "IO module libraries: ${LIBS}")
message(DEBUG "IO module definitions: ${COMPILE_DEFINITIONS}")
target_link_libraries(io PRIVATE ${LIBS})
target_include_directories(io PRIVATE ${INCLUDE_DIR} ${HEADERS})
