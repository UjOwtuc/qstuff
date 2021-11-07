set(qstuff_SRCS
	main.cpp
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


add_executable(qstuff ${qstuff_SRCS})
target_link_libraries(qstuff ${qstuff_LIBS})
install(TARGETS qstuff ${INSTALL_TARGETS_DEFAULT_ARGS})
