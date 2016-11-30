tests_path="`pwd`/tests"
SOURCES = $tests_path/../examples/server/PacketParser.cpp \
          $tests_path/../examples/server/Device.cpp \
          $$files($tests_path/../examples/server/packet/*.cpp) \
          $tests_path/test_shell.cpp \
          $tests_path/server_PacketParser_001.cpp

QMAKE_CXXFLAGS += -I$tests_path/../examples/server/include

# gcov
QMAKE_CXXFLAGS += -fprofile-arcs -ftest-coverage
LIBS += -lgcov -lpthread

# C++11
QMAKE_CXX = g++-5
QMAKE_LINK = g++-5
QMAKE_CC = gcc-5
QMAKE_CXXFLAGS += -std=c++11
