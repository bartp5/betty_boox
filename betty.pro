
#CONFIG		+= debug
TEMPLATE	= app
TARGET 		=
INCLUDEPATH 	+= . ../../libbetty/include
RESOURCES	= betty.qrc

HEADERS 	= mainwindow.h		\
		  terminalwidget.h	\
		  pty.h

SOURCES 	= betty.cpp 		\
		  mainwindow.cpp	\
		  terminalwidget.cpp	\
		  pty.cpp

LIBS		= ../../libbetty/lib/libbetty.a /opt/freescale/usr/local/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/arm-fsl-linux-gnueabi/multi-libs/armv5te/usr/lib/libutil.a
# LIBS		= ../../libbetty/lib/libbetty.a /usr/lib64/libutil.a
LIBS += -lz -lonyxapp -lonyx_base -lonyx_ui -lonyx_screen -lonyx_sys -lonyx_wpa -lonyx_wireless -lonyx_data -lonyx_cms

INCLUDEPATH += /opt/onyx/arm/include

