add_executable(test
  main.cc)

target_link_libraries(test PRIVATE Boost::headers)
