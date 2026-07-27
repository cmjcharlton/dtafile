# Add source to this project's executable.
add_executable (Summary "summary.cpp" "utility.cpp")
add_executable (Convert "convert.cpp")
add_executable (CreateTestData "createtestdata.cpp" "utility.cpp")
add_executable (dta2csv "dta2csv.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.20)
  set_property(TARGET Summary PROPERTY CXX_STANDARD 23)
  set_property(TARGET Convert PROPERTY CXX_STANDARD 23)
  set_property(TARGET CreateTestData PROPERTY CXX_STANDARD 23)
  set_property(TARGET dta2csv PROPERTY CXX_STANDARD 23)
endif()