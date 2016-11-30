SOURCES = ../examples/server/PacketParser.cpp ../examples/server/Device.cpp  ../examples/server/packet/* ./test_shell.cpp ./server_PacketParser_001.cpp
HEADERS = ../examples/server/include/* ../examples/server/include/packet/*

QMAKE_CXXFLAGS += -Wall -Wextra -Weffc++ -Werror

# gcov
QMAKE_CXXFLAGS += -fprofile-arcs -ftest-coverage
LIBS += -lgcov -lpthread

# C++11
QMAKE_CXX = g++-5
QMAKE_LINK = g++-5
QMAKE_CC = gcc-5
QMAKE_CXXFLAGS += -std=c++11
