set(SOURCES)
set(LIBS
    CSICS::queue
)

if (UNIX)
    list(APPEND SOURCES
        platform/unix/TCPEndpointUnix.cpp
        platform/unix/UDPEndpointUnix.cpp
        platform/unix/NetTypes.cpp
    )
endif()

if (CSICS_USE_MQTT)
    list(APPEND SOURCES
        MQTTEndpoint.cpp
    )
    list(APPEND LIBS
        paho-mqtt-c
    )
endif()

add_library(net OBJECT
    ${SOURCES}
)
target_link_libraries(net PRIVATE ${LIBS})
target_compile_definitions(net PRIVATE ${CSICS_COMPILE_DEFINITIONS})
target_compile_options(net PRIVATE ${CSICS_COMPILE_FLAGS})
target_link_options(net PRIVATE ${CSICS_LINKER_FLAGS})
target_include_directories(net PUBLIC ${INCLUDE_DIR})
