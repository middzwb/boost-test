set(BOOST_TEST_SRC
    test.cc)

add_executable(btest ${BOOST_TEST_SRC})

set(BOOST_COMPONENTS
    system context coroutine)
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

set(boost_test_deps
    Boost::system
    Boost::context
    Boost::coroutine
    ${CMAKE_THREAD_LIBS_INIT})

target_link_libraries(btest ${boost_test_deps})
#target_link_libraries(btest ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
