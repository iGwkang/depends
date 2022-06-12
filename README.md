# depends
Commonly used C++ open source libraries, most of them are head-only libraries.

* [depends](#depends)
  * [Network Library](#network-library)
  * [Json Library](#json-library)
  * [Log Library](#log-library)
  * [Command Line Library](#command-line-library)
  * [Common](#common)

## Network Library

- Asio https://think-async.com/Asio/

  > Asio is a cross-platform C++ library for network and low-level I/O programming that provides developers with a consistent asynchronous model using a modern C++ approach.

## Json Library

- nlohmann https://github.com/nlohmann/json

  > JSON for Modern C++

## Log Library

- spdlog https://github.com/gabime/spdlog

  > Very fast, header-only/compiled, C++ logging library.

## Command Line Library

- cxxopts https://github.com/jarro2783/cxxopts

  > Lightweight C++ command line option parser

## Common

- `log.h log.cpp` Customized log format and initialization-related operations.

- `defer.hpp` Implemented similar to defer in go.

- `byteorder.h` Byte order conversion.

- `spin_lock.h` Spin Lock, implemented using `std::atomic_flag`.

- `thread_pool.h` Grouped task pool.

- `timer.hpp` Timer implemented using `std::priority_queue` and `std::condition_variable`

- `functional_ex.hpp` C++14 lambda implements bind_front. **Known issue: The default parameter is passed into the non-copyable parameter, and the formal parameter must be a reference**
