project(test-dummy)

add_executable(${PROJECT_NAME} ${PROJECT_NAME}.cpp)
target_link_libraries(${PROJECT_NAME} pmt Catch2::Catch2WithMain)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}
                                                   ${CMAKE_SOURCE_DIR})
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
