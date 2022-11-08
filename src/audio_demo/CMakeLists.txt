find_package(fmt REQUIRED)

add_executable(audio_demo main.cpp)

target_link_libraries(audio_demo PRIVATE audio fmt::fmt)
