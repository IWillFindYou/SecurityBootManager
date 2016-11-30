SOURCES = `pwd`/tests/../examples/server/PacketParser.cpp \
          `pwd`/tests/../examples/server/Device.cpp \
          $$files(`pwd`/tests/../examples/server/packet/*.cpp) \
          `pwd`/tests/test_shell.cpp \
          `pwd`/tests/server_PacketParser_001.cpp

QMAKE_CXXFLAGS += -I`pwd`/tests/../examples/server/include

# gcov
QMAKE_CXXFLAGS += -fprofile-arcs -ftest-coverage
LIBS += -lgcov -lpthread

# C++11
QMAKE_CXX = g++
QMAKE_LINK = g++
QMAKE_CC = gcc
