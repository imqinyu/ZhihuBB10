APP_NAME = AuthProbe
CONFIG += qt warn_on cascades10
QT += network declarative
LIBS += -lbbdata -lbbsystem
INCLUDEPATH += ../../src
SOURCES += main.cpp ../../src/zhihu.cpp ../../src/signature.cpp
HEADERS += probe.hpp ../../src/zhihu.hpp ../../src/signature.hpp
