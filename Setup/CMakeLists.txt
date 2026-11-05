if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.23)
    project(ArgusSetup VERSION 0.4.1 LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

add_executable(ArgusSetup Private/Main.cpp)
target_include_directories(ArgusSetup PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(ArgusSetup PRIVATE Advapi32 Shell32 Ole32 Newdev Netapi32)
target_link_options(ArgusSetup PRIVATE "/MANIFESTUAC:level='requireAdministrator' uiAccess='false'")

if(TARGET ArgusAgent AND TARGET ArgusKernelPackage)
    add_custom_target(ArgusBundle
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/Bundle/$<CONFIG>/Driver"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:ArgusSetup>" "${CMAKE_BINARY_DIR}/Bundle/$<CONFIG>/ArgusSetup.exe"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:ArgusAgent>" "${CMAKE_BINARY_DIR}/Bundle/$<CONFIG>/ArgusAgent.exe"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${PROJECT_SOURCE_DIR}/.build/signing/ArgusTest.cer" "${CMAKE_BINARY_DIR}/Bundle/$<CONFIG>/ArgusTest.cer"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_BINARY_DIR}/Kernel/$<CONFIG>/ArgusKernelPackage" "${CMAKE_BINARY_DIR}/Bundle/$<CONFIG>/Driver"
        DEPENDS ArgusSetup ArgusAgent ArgusKernelPackage
        VERBATIM
    )

    find_program(ARGUS_WIX_EXECUTABLE NAMES wix wix.exe
        HINTS "${PROJECT_SOURCE_DIR}/.build/wix4-tools")
    if(ARGUS_WIX_EXECUTABLE)
        add_custom_target(ArgusMsi
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/Msi/$<CONFIG>"
            COMMAND "${ARGUS_WIX_EXECUTABLE}" build -arch x64
                -d "ArgusVersion=${PROJECT_VERSION}"
                -d "BundleDir=${CMAKE_BINARY_DIR}/Bundle/$<CONFIG>"
                -out "${CMAKE_BINARY_DIR}/Msi/$<CONFIG>/Argus-${PROJECT_VERSION}-x64.msi"
                "${CMAKE_CURRENT_SOURCE_DIR}/Wix/Argus.wxs"
            DEPENDS ArgusBundle
            VERBATIM
        )
    else()
        message(STATUS "WiX Toolset not found: install wix.exe and reconfigure to enable ArgusMsi")
    endif()
endif()
