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

add_executable(ArtTests ./art_tests.cpp)
target_link_libraries(ArtTests ArtLib doctest)
add_test(NAME test_art COMMAND ArtTests)

add_executable(ArtNodeTests ./node_tests.cpp)
target_link_libraries(ArtNodeTests ArtLib doctest)
add_test(NAME test_art_node COMMAND ArtNodeTests)

add_executable(ArtInnerNodeTests ./inner_node_tests.cpp)
target_link_libraries(ArtInnerNodeTests ArtLib doctest)
add_test(NAME test_art_inner_node COMMAND ArtInnerNodeTests)

add_executable(ArtNode4Tests ./node_4_tests.cpp)
target_link_libraries(ArtNode4Tests ArtLib doctest)
add_test(NAME test_art_node_4 COMMAND ArtNode4Tests)

add_executable(ArtNode16Tests ./node_16_tests.cpp)
target_link_libraries(ArtNode16Tests ArtLib doctest)
add_test(NAME test_art_node_16 COMMAND ArtNode16Tests)

add_executable(ArtNode48Tests ./node_48_tests.cpp)
target_link_libraries(ArtNode48Tests ArtLib doctest)
add_test(NAME test_art_node_48 COMMAND ArtNode48Tests)

add_executable(ArtNode256Tests ./node_256_tests.cpp)
target_link_libraries(ArtNode256Tests ArtLib doctest)
add_test(NAME test_art_node_256 COMMAND ArtNode256Tests)

add_executable(ArtTreeItTests ./tree_it_tests.cpp)
target_link_libraries(ArtTreeItTests ArtLib doctest)
add_test(NAME test_art_tree_it COMMAND ArtTreeItTests)

