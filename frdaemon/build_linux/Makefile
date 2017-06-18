
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
	../src/redis-cplusplus-client/anet.c \
	../src/recognition/caffe_person_recognition.cpp \
	../src/recognition/caffe_binding.cpp \
	../src/recognition/util/bounding_box.cpp \
	../src/recognition/util/help_functions.cpp 


CPPFLAGS 	= -std=c++14 -D_DEFAULT_SOURCE 

ifdef DEBUG
CPPFLAGS 	+= -D_DEBUG -Og -ggdb -Wall -g
endif

#CPPFLAGS 	+= -DUSE_DAEMON

LIBS 		+= -lpthread
LIBS 		+= -lboost_system -lboost_filesystem -lboost_thread
LIBS 		+= -lcurl
LIBS 		+= -lopencv_core -lopencv_imgproc -lopencv_video -lopencv_highgui
LIBS 		+= -lodbc
LIBS 		+= -luuid
LIBS		+= -lglog
LIBS		+=  -L../src/caffe/build/lib -lcaffe

INCLUDES	+= -I../src/caffe/build/src/caffe/proto
INCLUDES	+= -I../src/caffe/include


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

