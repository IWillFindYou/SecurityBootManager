#!/bin/bash
g++ -I./include ThreadPool.cpp Device.cpp Server.cpp PacketParser.cpp PingThread.cpp ExecuteThread.cpp TestServer.cpp packet/*.cpp -o TestServer -lpthread
