set(ARGUS_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
file(GLOB_RECURSE ARGUS_CORE_PUBLIC_FILES CONFIGURE_DEPENDS
    "${ARGUS_CORE_DIR}/Public/*.h"
    "${ARGUS_CORE_DIR}/Public/*.hpp"
    "${ARGUS_CORE_DIR}/Public/*.inl"
)
file(GLOB_RECURSE ARGUS_CORE_PRIVATE_FILES CONFIGURE_DEPENDS
    "${ARGUS_CORE_DIR}/Private/*.h"
    "${ARGUS_CORE_DIR}/Private/*.hpp"
    "${ARGUS_CORE_DIR}/Private/*.inl"
)
set(ARGUS_CORE_FILES ${ARGUS_CORE_PUBLIC_FILES} ${ARGUS_CORE_PRIVATE_FILES})
file(GLOB_RECURSE ARGUS_CORE_SOURCES CONFIGURE_DEPENDS
    "${ARGUS_CORE_DIR}/Private/*.cpp"
)

add_library(ArgusCore STATIC ${ARGUS_CORE_SOURCES} ${ARGUS_CORE_FILES})
add_library(Argus::Core ALIAS ArgusCore)
set_target_properties(ArgusCore PROPERTIES FOLDER "Core" OUTPUT_NAME "Core")

source_group(
    TREE "${ARGUS_CORE_DIR}"
    PREFIX "Core"
    FILES ${ARGUS_CORE_SOURCES} ${ARGUS_CORE_FILES}
)

target_include_directories(ArgusCore PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
)
target_include_directories(ArgusCore PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Private"
)
target_link_libraries(ArgusCore PUBLIC ArgusCompilerOptions)

