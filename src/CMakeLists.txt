set(qstuff_SRCS
	main.cpp
	mainwindow.cpp
	percentbardelegate.cpp
	logmodel.cpp
	timeinputdialog.cpp
	timerangemodel.cpp
	saveviewdialog.cpp
)
add_executable(qstuff ${qstuff_SRCS})
target_link_libraries(qstuff Qt5::Gui Qt5::Widgets Qt5::Network Qt::Charts)
install(TARGETS qstuff ${INSTALL_TARGETS_DEFAULT_ARGS})
