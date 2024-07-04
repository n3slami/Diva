add_executable(SteroidsTests ./steroids_tests.cpp)
target_link_libraries(SteroidsTests SteroidsLib)
add_test(NAME test_steroids COMMAND SteroidsTests)


add_test(NAME test_all COMMAND SteroidsTests)

