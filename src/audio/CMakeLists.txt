find_package(Boost REQUIRED)
find_package(fmt REQUIRED)
find_package(portaudio REQUIRED)
find_package(spdlog REQUIRED)

add_library(audio audio.cpp)
target_compile_options(audio PRIVATE ${WARN_FLAGS})
target_link_libraries(audio PRIVATE Boost::boost fmt::fmt portaudio spdlog::spdlog)
target_include_directories(audio PUBLIC include)
