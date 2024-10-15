project(pmt-powersensor3)

# Enable OpenMP
find_package(OpenMP REQUIRED)

set(POWERSENSOR3_ROOT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/PowerSensor3/host/src)
set(POWERSENSOR3_ROOT_INC_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/PowerSensor3/host/include)
set(POWERSENSOR3_SOURCE ${POWERSENSOR3_ROOT_SRC_DIR}/PowerSensor.cc
                        ${POWERSENSOR3_ROOT_SRC_DIR}/sensors.cc)

include(AddSensor)

set(LINK_LIBRARIES OpenMP::OpenMP_C)
set(INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR} ${POWERSENSOR3_ROOT_INC_DIR})

add_sensor(
  SENSOR_NAME
  "powersensor3"
  HEADER
  PowerSensor3.h
  SRC_FILES
  PowerSensor3.cpp
  ${POWERSENSOR3_SOURCE}
  LINK_LIBRARIES
  ${LINK_LIBRARIES}
  INCLUDE_DIRECTORIES
  ${INCLUDE_DIRECTORIES})
