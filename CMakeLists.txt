set(CMAKE_CXX_STANDARD 17)
AC_ADD_SCRIPT("${CMAKE_CURRENT_LIST_DIR}/src/FlightPathDiscovery.cpp")
install(FILES conf/mod-flightpath-discovery.conf.dist DESTINATION etc/modules)
