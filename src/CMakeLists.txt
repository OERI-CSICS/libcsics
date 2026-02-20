
add_library(core INTERFACE)

add_library(CSICS::core ALIAS core)

if (CSICS_BUILD_QUEUE)
    add_library(queue STATIC queue/SPSCQueue.cpp)
    target_include_directories(queue PUBLIC ${INCLUDE_DIR})
    add_library(CSICS::queue ALIAS queue)
    target_compile_options(queue PRIVATE ${CSICS_COMPILE_FLAGS})
    target_link_options(queue PRIVATE ${CSICS_LINKER_FLAGS})
    target_compile_definitions(queue PRIVATE ${CSICS_COMPILE_DEFINITIONS})
endif()

if (CSICS_BUILD_RADIO)
    add_subdirectory(radio)
    add_library(CSICS::radio ALIAS radio)
endif()

if (CSICS_BUILD_IO)
    add_subdirectory(io)
    add_library(CSICS::io ALIAS io)
endif()

if (CSICS_BUILD_SERIALIZATION)
    add_subdirectory(serialization)
    add_library(CSICS::serialization ALIAS serialization)
endif()

if (CSICS_BUILD_GEO)
    add_subdirectory(geo)
    add_library(CSICS::geo ALIAS geo)
endif()
