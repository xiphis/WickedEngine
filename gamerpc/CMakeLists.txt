PROJECT(gamerpc)

find_package(gflags CONFIG REQUIRED)
find_package(glog CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Libnghttp2 REQUIRED)

subdirs(plugin)

#file(GLOB ProtoFiles "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")
#PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders ${ProtoFiles})

set(PROTO_FILES
    proto/gamerpc.proto
)



add_library(gamerpc STATIC
    ${PROTO_FILES}
    #${ProtoSources} ${ProtoHeaders}
    dummy.cpp
)

target_include_directories(gamerpc PRIVATE ${LIBNGHTTP2_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate(TARGET gamerpc LANGUAGE cpp)


target_link_libraries(gamerpc PRIVATE glog::glog gflags::gflags)
target_link_libraries(gamerpc PRIVATE TBB::tbb TBB::tbbmalloc TBB::tbbmalloc_proxy)
target_link_libraries(gamerpc PRIVATE protobuf::libprotobuf)
target_link_libraries(gamerpc PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(gamerpc PRIVATE ${LIBNGHTTP2_LIBRARIES})

