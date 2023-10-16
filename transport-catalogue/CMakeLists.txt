cmake_minimum_required(VERSION 3.10)

project(TransportCatalogue CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto)

set(CATALOGUE_FILES domain.cpp domain.hpp
                    geo.cpp geo.hpp
                    graph.hpp 
                    input_reader.cpp input_reader.hpp
                    json_builder.cpp json_builder.hpp
                    json.cpp json.hpp
                    json_reader.cpp json_reader.hpp
                    main.cpp map_renderer.cpp
                    map_renderer.hpp ranges.hpp
                    request_handler.cpp request_handler.hpp
                    router.hpp serialization.cpp
                    serialization.hpp stat_reader.cpp
                    stat_reader.hpp svg.cpp
                    svg.hpp transport_catalogue.cpp
                    transport_catalogue.hpp transport_router.cpp
                    transport_router.hpp)

add_executable(transport_catalogue ${CATALOGUE_FILES} ${PROTO_HDRS} ${PROTO_SRCS})

target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads)
