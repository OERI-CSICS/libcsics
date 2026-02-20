
set(SOURCES
    Ops.cpp
)

set(LIBS
    ${GeographicLib_LIBRARIES}
)

set(INCLUDES
    ${GeographicLib_INCLUDE_DIRS}
)

add_library(geo ${SOURCES})
target_link_libraries(geo ${LIBS})
target_include_directories(geo PRIVATE ${INCLUDES} PUBLIC ${INCLUDE_DIR})
target_compile_options(geo PRIVATE ${CSICS_COMPILE_FLAGS})
target_link_options(geo PRIVATE ${CSICS_LINKER_FLAGS})
target_compile_definitions(geo PRIVATE ${CSICS_COMPILE_DEFINITIONS})


