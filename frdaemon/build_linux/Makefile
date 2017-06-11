
TARGET = frdaemon

SOURCES = \
	../src/frd.cpp \
	../src/person.cpp \
	../src/redis_client.cpp \
	../src/ftp_client.cpp \
	../src/frame_features.cpp \
	../src/redis-cplusplus-client/anet.c


CPPFLAGS 	= -std=c++14 -D_DEFAULT_SOURCE 

#CPPFLAGS 	+= -DUSE_DAEMON

#CPPFLAGS 	+= -g -Wall -O0 -D_DEBUG -ggdb 


LIBS 		+= -lpthread
LIBS 		+= -lboost_system -lboost_filesystem
LIBS 		+= -lcurl
LIBS 		+= -lopencv_core -lopencv_imgproc -lopencv_video -lopencv_highgui

all: 	$(TARGET)


$(TARGET): $(SOURCES)
	g++ $(SOURCES) $(CPPFLAGS) $(INCLUDES) $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)

prepare:
	apt-get update	
	apt-get install -y g++
	apt-get install -y libboost-system-dev libboost-filesystem-dev 
	apt-get install -y libcurl4-openssl-dev 
	apt-get install -y libopencv-dev 

install:

