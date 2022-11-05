find_package(portaudio REQUIRED)
find_package(fmt REQUIRED)

add_executable(audio_demo main.cpp)

target_link_libraries(audio_demo PRIVATE portaudio fmt::fmt)
