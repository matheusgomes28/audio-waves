find_package(fmt REQUIRED)
find_package(Boost REQUIRED)

add_executable(audio_demo main.cpp)

target_link_libraries(audio_demo PRIVATE audio fmt::fmt Boost::boost)
