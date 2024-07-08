add_executable(SteroidsTests ./steroids_tests.cpp)
target_link_libraries(SteroidsTests SteroidsLib)
add_test(NAME test_steroids COMMAND SteroidsTests)

add_executable(InfixStoreTests ./infix_store_tests.cpp)
target_link_libraries(InfixStoreTests SteroidsLib)
add_test(NAME test_infix_store COMMAND InfixStoreTests)


add_test(NAME test_all COMMAND SteroidsTests
                               InfixStoreTests)

