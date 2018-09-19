TARGET = CalcScripting

TEMPLATE = lib

include(GammaRay.pri)

#This prevents "string table overflow" errors when compiling .cpp's that include exprtk.hpp in debug mode
QMAKE_CXXFLAGS_DEBUG += -O1

win32 {
	#Currently not a recognized (or possibly not necessary) option in GCC 4.8 (Linux)
	QMAKE_CXXFLAGS += -Wa,-mbig-obj
}

SOURCES += calculator/calcscripting.cpp \
           calculator/icalcproperty.cpp \
           calculator/icalcpropertycollection.cpp

HEADERS += calculator/calcscripting.h \
           calculator/icalcproperty.h \
           calculator/icalcpropertycollection.h

#=====================Embedded thirdparty libraries===========================
INCLUDEPATH += thirdparty
#=============================================================================

# The library version
VERSION = 1.0.0

# Define a preprocessor macro so we can get the application version in application code.
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

# Define application name macro
DEFINES += APP_NAME=\\\"$$TARGET\\\"

# Define application name and version macro
# \040 means a whitespace.  Don't replace it with an explicit space because
# it doesn't compile.
DEFINES += APP_NAME_VER=\\\"$$TARGET\\\040$$VERSION\\\"

# To signal whether we are compiling the Calculator Scriptin library.
DEFINES += IS_COMPILING_CALCSCRIPT_LIB
