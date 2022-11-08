find_package(fmt REQUIRED)
find_package(portaudio REQUIRED)
find_package(spdlog REQUIRED)

add_library(audio audio.cpp)
target_link_libraries(audio PRIVATE fmt::fmt portaudio spdlog::spdlog)
target_include_directories(audio PUBLIC include)
