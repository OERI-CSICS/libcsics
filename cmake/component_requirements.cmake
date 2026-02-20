# Defines the requirements for a component.
# Eg; radio depends on queue, etc etc.

if (CSICS_BUILD_RADIO AND NOT CSICS_BUILD_QUEUE)
    message(FATAL_ERROR "Radio component requires Queue component. Please enable CSICS_BUILD_QUEUE.")
endif()

if (CSICS_BUILD_GEO AND NOT CSICS_BUILD_LINALG )
    message(FATAL_ERROR "Geo component requires Linalg component. Please enable CSICS_BUILD_LINALG.")
endif()

