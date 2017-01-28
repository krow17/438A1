# CSCE 438: Distributed objects programming
# HW1: A Chat Room Service
# 
# makefile
# Vincent Velarde, Kyle Rowland
# January 2017
#
# NOTES:
#  "make" to compile server and client programs
#  "clean" to remove server and client programs
#

all: server client

client: crc.cpp
	g++ -o client crc.cpp

server: crsd.cpp
	g++ -o server crsd.cpp

clean:
	\rm *.o server client
