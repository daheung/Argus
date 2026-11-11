# CMake's normal MSVC defaults target user mode and pull in the CRT.
string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_STANDARD_LIBRARIES "")

# Compile the portable Core implementations with the kernel toolset.
set(ARGUS_KERNEL_CORE_SHARED_SOURCES
    "${PROJECT_SOURCE_DIR}/Core/Private/Core.cpp"
    "${PROJECT_SOURCE_DIR}/Core/Private/Misc/Assertion.cpp"
    "${PROJECT_SOURCE_DIR}/Core/Private/Misc/GenericStricmp.cpp"
    "${PROJECT_SOURCE_DIR}/Core/Private/Misc/GenericString.cpp"
)

file(GLOB_RECURSE ARGUS_KERNEL_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Public/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Public/*.inl"
    "${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Private/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Private/*.inl"
)
file(GLOB_RECURSE ARGUS_KERNEL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp"
)

add_library(ArgusKernel MODULE
    ${ARGUS_KERNEL_CORE_SHARED_SOURCES}
    ${ARGUS_KERNEL_SOURCES}
    ${ARGUS_KERNEL_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/ArgusKernel.natvis"
)
source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}"
    PREFIX "Source"
    FILES ${ARGUS_KERNEL_SOURCES} ${ARGUS_KERNEL_FILES}
)
source_group(
    TREE "${PROJECT_SOURCE_DIR}/Core"
    PREFIX "Core"
    FILES ${ARGUS_KERNEL_CORE_SHARED_SOURCES}
)
set_target_properties(ArgusKernel PROPERTIES
    PREFIX ""
    SUFFIX ".sys"
    MSVC_RUNTIME_LIBRARY "MultiThreaded"
    VS_PLATFORM_TOOLSET "WindowsKernelModeDriver10.0"
    VS_GLOBAL_DriverType "WDM"
    VS_GLOBAL_SignMode "Off"
    VS_GLOBAL_SupportsPackaging "false"
)

target_link_libraries(ArgusKernel PRIVATE ArgusCompilerOptions
    "${ARGUS_WDK_LIB_DIR}/ntoskrnl.lib"
    "${ARGUS_WDK_LIB_DIR}/fltmgr.lib"
    "${ARGUS_WDK_LIB_DIR}/Wdmsec.lib"
    "${ARGUS_WDK_LIB_DIR}/BufferOverflowK.lib"
    # TODO: Remove this when the shared Core character helpers no longer require CRT functions.
    "${ARGUS_WDK_LIB_DIR}/libcntpr.lib"
)
target_include_directories(ArgusKernel PRIVATE
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/Core/Private"
    "${CMAKE_CURRENT_SOURCE_DIR}/Private"
)
target_include_directories(ArgusKernel SYSTEM PRIVATE
    "${ARGUS_WDK_INCLUDE_DIR}/km"
    "${ARGUS_WDK_INCLUDE_DIR}/shared"
)
target_compile_definitions(ArgusKernel PRIVATE _AMD64_=1 AMD64=1)
target_compile_options(ArgusKernel PRIVATE /kernel /Zl)
target_link_options(ArgusKernel PRIVATE
    /KERNEL
    /DRIVER
    /INTEGRITYCHECK
    /SUBSYSTEM:NATIVE
    /ENTRY:DriverEntry
    /NODEFAULTLIB
    /MANIFEST:NO
    /INCREMENTAL:NO
)

# Sign the distributable .sys and .cat in a separate package target after linking.
# WDK's automatic signing is disabled above so both files use the same test key.
find_program(ARGUS_INF2CAT_EXECUTABLE NAMES Inf2Cat.exe
    HINTS "${ARGUS_WDK_ROOT}/bin/${ARGUS_WDK_VERSION}/x86"
    REQUIRED
)
find_program(ARGUS_SIGNTOOL_EXECUTABLE NAMES signtool.exe
    HINTS "${ARGUS_WDK_ROOT}/bin/${ARGUS_WDK_VERSION}/x64"
    REQUIRED
)
find_program(ARGUS_OPENSSL_EXECUTABLE NAMES openssl.exe
    HINTS "C:/Program Files/OpenSSL-Win64/bin"
)

add_custom_target(ArgusKernelPackage ALL
    COMMAND "${CMAKE_COMMAND}"
        "-DARGUS_PACKAGE_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/ArgusKernelPackage"
        "-DARGUS_SYS_FILE=$<TARGET_FILE:ArgusKernel>"
        "-DARGUS_INF_FILE=${CMAKE_CURRENT_SOURCE_DIR}/ArgusKernel.inf"
        "-DARGUS_INF2CAT_EXE=${ARGUS_INF2CAT_EXECUTABLE}"
        "-DARGUS_SIGNTOOL_EXE=${ARGUS_SIGNTOOL_EXECUTABLE}"
        "-DARGUS_OPENSSL_EXE=${ARGUS_OPENSSL_EXECUTABLE}"
        "-DARGUS_SIGNING_DIR=${PROJECT_SOURCE_DIR}/.build/signing"
        -P "${PROJECT_SOURCE_DIR}/cmake/ArgusKernelPackage.cmake"
    DEPENDS ArgusKernel
    VERBATIM
)
set_target_properties(ArgusKernelPackage PROPERTIES FOLDER "Packaging")
