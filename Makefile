CXXFLAGS += -std=c++11
 
CC 	= gcc
CPP = g++

COMPLIE_INCL = -I./include -I./packed -I./packed/pj

COMPLIE_LIB  = -L./ -lpthread
CFLAGS 		= -Wall -DHAVE_CONFIG_H -g -fPIC #-DHAVE_SYS_TIME_H
CXXFLAGS += $(COMPLIE_INCL) 
LDFLAGS += $(COMPLIE_LIB)

CFLAGS  += -I./packed
CFLAGS	+= $(COMPLIE_INCL) 

OBJECTS = NALDecoder.o UnitTest.o transport_udp.o packed/packets_list.o packed/h264_packetizer.o
EXE=UnitTest
 
 
$(EXE):$(OBJECTS)
	$(CC) -o $(EXE) $(OBJECTS) $(LDFLAGS) 

.PHONY:clean
clean:
	rm $(EXE) $(OBJECTS)
