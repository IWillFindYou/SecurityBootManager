SOURCES = ../examples/server/PacketParser.cpp ../examples/server/Device.cpp  $$files(../examples/server/packet/*.cpp) ./test_shell.cpp ./server_PacketParser_001.cpp

QMAKE_CXXFLAGS += -Wall -Wextra -Weffc++ -Werror
QMAKE_CXXFLAGS += -I../examples/server/include/

# gcov
QMAKE_CXXFLAGS += -fprofile-arcs -ftest-coverage
LIBS += -lgcov -lpthread

# C++11
QMAKE_CXX = g++
QMAKE_LINK = g++
QMAKE_CC = gcc
