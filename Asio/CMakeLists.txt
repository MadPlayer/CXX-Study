add_executable(asio_shared_echo
  shared_ptr_main.cc)

add_executable(asio_unique_echo
  unique_ptr_main.cc)

target_link_libraries(asio_shared_echo PRIVATE Boost::headers)
target_link_libraries(asio_unique_echo PRIVATE Boost::headers)
