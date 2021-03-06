CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project (UnitTests)

include_directories (BEFORE ${CMAKE_CURRENT_BINARY_DIR})

add_executable (basic_rag_test Rag/basic_rag.cpp)
add_executable (basic_stack_test Stack/basic_stack.cpp)

set (json_LIB jsoncpp)
set (hdf5_LIBRARIES hdf5 hdf5_hl)
set (boost_LIBS boost_thread boost_system boost_program_options boost_unit_test_framework boost_filesystem)
set (PYTHON_LIBRARY_FILE ${PYTHON_LIBRARIES})
set (vigra_LIB vigraimpex)
set (libdvid_LIBS ${LIBDVIDCPP_LIBRARY})

target_link_libraries (basic_rag_test Stack FeatureManager Rag IO ${vigra_LIB} ${hdf5_LIBRARIES} ${json_LIB} ${boost_LIBS} ${libdvid_LIBS} ${PYTHON_LIBRARY_FILE})
target_link_libraries (basic_stack_test Stack FeatureManager Rag IO ${vigra_LIB} ${hdf5_LIBRARIES} ${libdvid_LIBS} ${json_LIB} ${boost_LIBS} ${PYTHON_LIBRARY_FILE})

if (NOT ${CMAKE_SOURCE_DIR} STREQUAL ${BUILDLOC})  
    add_custom_command (
        TARGET basic_rag_test 
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy basic_rag_test ${CMAKE_SOURCE_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E remove basic_rag_test)

    add_custom_command (
        TARGET basic_stack_test 
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy basic_stack_test ${CMAKE_SOURCE_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E remove basic_stack_test)
endif()

add_test ("simple_rag_unit_tests" ${CMAKE_SOURCE_DIR}/bin/basic_rag_test)

add_test ("simple_stack_unit_tests"
        ${CMAKE_SOURCE_DIR}/bin/basic_stack_test
        ${CMAKE_SOURCE_DIR}/unit_tests/Stack/samp1_labels.h5
        ${CMAKE_SOURCE_DIR}/unit_tests/Stack/samp1_prediction.h5
        )
