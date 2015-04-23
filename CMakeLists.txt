cmake_minimum_required(VERSION 2.8)
PROJECT(animtool)
FIND_PACKAGE(Qt4 REQUIRED QtCore QtGui QtXml)

SET( source
	src/main.cpp
	src/animtool.cpp
	src/view.cpp
	src/tablemodel.cpp
	src/animation.cpp
	src/project.cpp
	src/projectfile.cpp
	src/xcf.cpp
	src/command.cpp
	src/partcommands.cpp
	src/animationcommands.cpp
	src/editcommands.cpp
	src/ikcommands.cpp
	src/export.cpp
	src/ik.cpp
)

SET( headers
	src/animation.h
	src/animtool.h
	src/part.h
	src/project.h
	src/tablemodel.h
	src/ui_anim.h
	src/view.h
	src/xcf.h
	src/command.h
	src/partcommands.h
	src/animationcommands.h
	src/editcommands.h
	src/ikcommands.h
	src/export.h
	src/ik.h
)

# Headers using Q_OBJECT macro
SET( qtheaders	
	src/animtool.h
	src/tablemodel.h
	src/view.h
	src/project.h
	src/command.h
	src/export.h
)

SET( forms
	ui/anim.ui
	ui/export.ui
)

SET( resource
	ui/anim.qrc
)

QT4_WRAP_CPP( moc ${qtheaders} )
QT4_WRAP_UI( form_h ${forms} )
QT4_ADD_RESOURCES( rcc ${resource} )

INCLUDE( ${QT_USE_FILE} )
ADD_DEFINITIONS( ${QT_DEFINITIONS} )

SET(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS, "-static-libgcc")

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR} src/ )		#allow g++ to find generated headers
ADD_EXECUTABLE( run ${source} ${moc} ${form_h} ${rcc} )
TARGET_LINK_LIBRARIES( run ${QT_LIBRARIES} )

