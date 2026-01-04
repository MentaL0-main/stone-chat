cmake_minimum_required(VERSION 3.20)
project(EliteChat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(Boost REQUIRED COMPONENTS system)

add_executable(chat_server chat_server.cpp)
target_link_libraries(chat_server 
    PRIVATE 
    Threads::Threads 
    ${Boost_LIBRARIES} 
    boost_system
)
target_include_directories(chat_server PRIVATE ${Boost_INCLUDE_DIRS})
target_compile_options(chat_server PRIVATE ${Boost_CFLAGS_OTHER})

target_compile_features(chat_server PRIVATE cxx_std_20)
target_compile_definitions(chat_server PRIVATE BOOST_ASIO_NO_DEPRECATED)

install(TARGETS chat_server DESTINATION bin)
