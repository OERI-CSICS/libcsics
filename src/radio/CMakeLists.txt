set(
    SOURCES 
    RadioRx.cpp 
    Radio.cpp
)
set(LIBRARIES queue)
set(DEFINITIONS ${CSICS_COMPILE_DEFINITIONS})

if (CSICS_USE_UHD)
    set(SOURCES ${SOURCES} usrp/USRPRadioRx.cpp)
    set(LIBRARIES ${LIBRARIES} uhd)
endif()

add_library(radio STATIC ${SOURCES})
target_include_directories(radio PUBLIC ${INCLUDE_DIR})
target_link_libraries(radio PUBLIC ${LIBRARIES})
target_compile_options(radio PRIVATE ${CSICS_COMPILE_FLAGS})
target_link_options(radio PRIVATE ${CSICS_LINKER_FLAGS})
target_compile_definitions(radio PUBLIC ${DEFINITIONS})

message(DEBUG "Radio module sources: ${SOURCES}")
message(DEBUG "Radio module libraries: ${LIBRARIES}")
message(DEBUG "Radio module definitions: ${DEFINITIONS}")
    
