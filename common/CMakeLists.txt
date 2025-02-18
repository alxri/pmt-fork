project(pmt-common)

add_library(${PROJECT_NAME} OBJECT PMT.cpp State.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${PMT_INCLUDE_DIR})

install(FILES PMT.h DESTINATION include/pmt/common)
install(FILES State.h Timestamp.h DESTINATION include/pmt/common)
