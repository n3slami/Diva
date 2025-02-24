# doctest
include(FetchContent)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
)
FetchContent_MakeAvailable(doctest)

# tests
add_executable(WormholeIntTests ./wormhole_int_tests.cpp)
target_link_libraries(WormholeIntTests WormholeIntLib doctest)
add_test(NAME test_wormhole_int COMMAND WormholeIntTests)

add_executable(InfixStoreTests ./infix_store_tests.cpp)
target_link_libraries(InfixStoreTests DivaLib doctest)
add_test(NAME test_infix_store COMMAND InfixStoreTests)

add_executable(DivaTests ./diva_tests.cpp)
target_link_libraries(DivaTests DivaLib doctest)
add_test(NAME test_diva COMMAND DivaTests)


