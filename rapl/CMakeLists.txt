project(pmt-rapl)

set(LINK_LIBRARIES Threads::Threads fmt-header-only)

add_sensor(
  SENSOR_NAME
  "rapl"
  HEADER
  Rapl.h
  SRC_FILES
  Rapl.cpp
  RaplImpl.cpp
  LINK_LIBRARIES
  ${LINK_LIBRARIES})
