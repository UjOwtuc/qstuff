set(qstuff_SRCS
	mainwindow.cpp
	percentbardelegate.cpp
	logmodel.cpp
	timeinputdialog.cpp
	timerangemodel.cpp
	saveviewdialog.cpp
	keyfilterproxymodel.cpp
	countschart.cpp
	filtermodel.cpp
	filterdelegate.cpp
	editfilterwidget.cpp
	queryvalidator.cpp
)
set(qstuff_LIBS
	Qt5::Gui Qt5::Widgets Qt5::Network Qt5::Charts
)

if(EXISTS ${logstuff_query})
	add_definitions(-DWITH_LOGSTUFF_QUERY)
	set(qstuff_LIBS ${qstuff_LIBS} ${logstuff_query})
endif()

add_library(libqstuff ${qstuff_SRCS})
target_link_libraries(libqstuff ${qstuff_LIBS})

add_executable(qstuff main.cpp)
target_link_libraries(qstuff ${qstuff_LIBS} libqstuff)
install(TARGETS qstuff ${INSTALL_TARGETS_DEFAULT_ARGS})

set(qstuff_TESTS
	test_queryvalidator.cpp
)
if(EXISTS ${Qt5Test_DIR})
	enable_testing()
	add_executable(qstuff_tests ${qstuff_TESTS})
	target_link_libraries(qstuff_tests Qt5::Test ${qstuff_LIBS} libqstuff)
	add_test(NAME qstuff_tests COMMAND qstuff_tests)
endif()
