QNX_HOST ?= C:/NDKFiles/host_10_3_1_12/win32/x86
QNX_TARGET ?= C:/NDKFiles/target_10_3_1_995/qnx6
export QNX_HOST
export QNX_TARGET
export PATH := $(QNX_HOST)/usr/bin:$(PATH)
QMAKE = $(QNX_HOST)/usr/bin/qmake.exe
MAKE_TOOL = $(QNX_HOST)/usr/bin/make.exe
DEV_SPEC = $(QNX_TARGET)/usr/share/qt4/mkspecs/blackberry-armv7le-qcc
SIM_SPEC = $(QNX_TARGET)/usr/share/qt4/mkspecs/blackberry-x86-qcc
.PHONY: all Device-Debug Device-Release Device-Profile Simulator-Debug
all: Device-Debug
Device-Debug:
	cd arm && "$(QMAKE)" -spec "$(DEV_SPEC)" CONFIG+=device CONFIG+=debug -o Makefile ../Zhihu.pro && "$(MAKE_TOOL)"
Device-Release: Device-Debug
Device-Profile: Device-Debug
Simulator-Debug:
	cd x86 && "$(QMAKE)" -spec "$(SIM_SPEC)" CONFIG+=simulator CONFIG+=debug -o Makefile ../Zhihu.pro && "$(MAKE_TOOL)"
