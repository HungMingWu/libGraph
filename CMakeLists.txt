project(graph)
add_executable(test test_main.cpp graph.cpp)
set_property(TARGET test PROPERTY CXX_STANDARD 17)
