
TARGET = frdaemon

ifdef DEBUG
TARGET = frdaemond
endif

SOURCES = \
	../src/frd.cpp \
	../src/person.cpp \
	../src/frame_features.cpp \
	../src/ftp_client.cpp \
	../src/mssql_client.cpp \
	../src/odbcxx/odbcxx.cpp \
	../src/redis_client.cpp \
	../src/redis-cplusplus-client/anet.c


CPPFLAGS 	= -std=c++11 -D_DEFAULT_SOURCE 

ifdef DEBUG
CPPFLAGS 	+= -D_DEBUG -Og -ggdb -Wall -g
endif

#CPPFLAGS 	+= -DUSE_DAEMON

LIBS 		+= -lpthread
LIBS 		+= -lboost_system -lboost_filesystem
LIBS 		+= -lcurl
LIBS 		+= -lopencv_core -lopencv_imgproc -lopencv_video -lopencv_highgui
LIBS 		+= -lodbc
LIBS 		+= -luuid

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
	apt-get install -y uuid-dev

	apt-get install -y unixodbc-dev	tdsodbc

	# 2do: this must be called only once
	odbcinst -i -d -f /usr/share/tdsodbc/odbcinst.ini

install:

