# doctest
set(DOCTEST_INCLUDE_DIR ./doctest/doctest/)
add_library(doctest INTERFACE)
target_include_directories(doctest INTERFACE ${DOCTEST_INCLUDE_DIR})

# tests
add_executable(SteroidsTests ./steroids_tests.cpp)
target_link_libraries(SteroidsTests SteroidsLib doctest)
add_test(NAME test_steroids COMMAND SteroidsTests)

add_executable(InfixStoreTests ./infix_store_tests.cpp)
target_link_libraries(InfixStoreTests SteroidsLib doctest)
add_test(NAME test_infix_store COMMAND InfixStoreTests)

add_executable(WormholeIntTests ./wormhole_int_tests.cpp)
target_link_libraries(WormholeIntTests WormholeIntLib doctest)
add_test(NAME test_wormhole_int COMMAND WormholeIntTests)


