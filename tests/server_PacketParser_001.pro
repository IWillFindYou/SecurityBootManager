SOURCES = ./examples/server/PacketParser.cpp \
          ./examples/server/Device.cpp \
          $$files(./examples/server/packet/*.cpp) \
          ./tests/test_shell.cpp \
          ./tests/server_PacketParser_001.cpp

QMAKE_CXXFLAGS += -I./examples/server/include

# gcov
QMAKE_CXXFLAGS += -fprofile-arcs -ftest-coverage
LIBS += -lgcov -lpthread

# C++11
QMAKE_CXX = g++
QMAKE_LINK = g++
QMAKE_CC = gcc
QMAKE_CXXFLAGS += -std=c++11
