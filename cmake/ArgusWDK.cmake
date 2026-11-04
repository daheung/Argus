set(ARGUS_WDK_ROOT "C:/Program Files (x86)/Windows Kits/10" CACHE PATH "Windows Driver Kit root")
set(ARGUS_WDK_VERSION "" CACHE STRING "WDK version (empty selects the newest installed version)")

if(NOT ARGUS_WDK_VERSION)
    file(GLOB _ArgusWdmHeaders "${ARGUS_WDK_ROOT}/Include/*/km/wdm.h")
    set(_ArgusWdkVersions)
    foreach(_ArgusWdmHeader IN LISTS _ArgusWdmHeaders)
        get_filename_component(_ArgusKmDirectory "${_ArgusWdmHeader}" DIRECTORY)
        get_filename_component(_ArgusIncludeDirectory "${_ArgusKmDirectory}" DIRECTORY)
        get_filename_component(_ArgusCandidateVersion "${_ArgusIncludeDirectory}" NAME)
        if(EXISTS "${ARGUS_WDK_ROOT}/Lib/${_ArgusCandidateVersion}/km/x64/ntoskrnl.lib")
            list(APPEND _ArgusWdkVersions "${_ArgusCandidateVersion}")
        endif()
    endforeach()
    list(SORT _ArgusWdkVersions COMPARE NATURAL ORDER DESCENDING)
    if(NOT _ArgusWdkVersions)
        message(FATAL_ERROR "No x64 WDK installation was found under ${ARGUS_WDK_ROOT}.")
    endif()
    list(GET _ArgusWdkVersions 0 ARGUS_WDK_VERSION)
endif()

set(ARGUS_WDK_INCLUDE_DIR "${ARGUS_WDK_ROOT}/Include/${ARGUS_WDK_VERSION}")
set(ARGUS_WDK_LIB_DIR "${ARGUS_WDK_ROOT}/Lib/${ARGUS_WDK_VERSION}/km/x64")

if(NOT EXISTS "${ARGUS_WDK_INCLUDE_DIR}/km/wdm.h"
    OR NOT EXISTS "${ARGUS_WDK_LIB_DIR}/ntoskrnl.lib"
    OR NOT EXISTS "${ARGUS_WDK_LIB_DIR}/BufferOverflowK.lib")
    message(FATAL_ERROR "WDK ${ARGUS_WDK_VERSION} was not found under ${ARGUS_WDK_ROOT}.")
endif()

message(STATUS "Argus WDK version: ${ARGUS_WDK_VERSION}")
