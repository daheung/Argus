add_executable(ArgusAgent Private/Main.cpp Private/Rules.cpp)

target_link_libraries(ArgusAgent PRIVATE Argus::Core Shell32 Ole32)
