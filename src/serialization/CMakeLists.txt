
set(SOURCES
    JSONSerializer.cpp
)

set(LIBS
)

set(INCLUDES
)

add_library(serialization ${SOURCES})
target_link_libraries(serialization ${LIBS})
target_include_directories(serialization PRIVATE ${INCLUDES} PUBLIC ${INCLUDE_DIR})
target_compile_options(serialization PRIVATE ${CSICS_COMPILE_FLAGS})
target_link_options(serialization PRIVATE ${CSICS_LINKER_FLAGS})
target_compile_definitions(serialization PRIVATE ${CSICS_COMPILE_DEFINITIONS})

