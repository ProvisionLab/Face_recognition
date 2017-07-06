
TARGET = crdaemon

ifdef DEBUG
TARGET = crdaemond
endif

SOURCES = \
	../src/crd.cpp \
	../src/frame.cpp \
	../src/redis_client.cpp \
	../src/redis-cplusplus-client/anet.c


CPPFLAGS 	= -std=c++11 -D_DEFAULT_SOURCE 

ifdef DEBUG
CPPFLAGS 	+= -D_DEBUG -Og -ggdb -Wall -g
endif

#CPPFLAGS 	+= -DUSE_DAEMON

LIBS 		+= -lpthread
LIBS 		+= -lboost_system -lboost_filesystem
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
	apt-get install -y libopencv-dev

install:
