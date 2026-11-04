add_executable(ArgusAgent Private/Main.cpp)

target_link_libraries(ArgusAgent PRIVATE Argus::Core)
