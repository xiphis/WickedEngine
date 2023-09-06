
find_package(Protobuf REQUIRED)
find_package(inja CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(gamerpc_plugin
    #cpp_generator.cpp
    proto_parser_helper.cpp
    main.cpp
)
target_link_libraries(gamerpc_plugin PRIVATE protobuf::libprotobuf protobuf::libprotoc)
target_link_libraries(gamerpc_plugin PRIVATE pantor::inja nlohmann_json::nlohmann_json)


