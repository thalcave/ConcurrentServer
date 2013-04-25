# Make sure the compiler can find include files from our common library. 
include_directories (${cserver_SOURCE_DIR}/common) 

# Make sure the linker can find the Hello library once it is built. 
link_directories (${cserver_BINARY_DIR}/common)


ADD_EXECUTABLE(
	client
	client.cpp
)

target_link_libraries(
	client
	common
)
