
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


INCLUDES += -I/home/greeser/Work/toolkits/opencv/build/install/include
INCLUDES += -I/home/greeser/Work/toolkits/openalpr-2.3.0/src/build/install/include

LIBS 		+= -lpthread
LIBS 		+= -lboost_system -lboost_filesystem
LIBS		+= -L/home/greeser/Work/toolkits/openalpr-2.3.0/src/build/install/lib -lopenalpr

LIBS 		+= -L/home/greeser/Work/toolkits/opencv/build/install/lib \
				-lopencv_core \
				-lopencv_imgproc \
				-lopencv_highgui \
				-lopencv_ml \
				-lopencv_video \
				-lopencv_features2d \
				-lopencv_calib3d \
				-lopencv_objdetect \
				-lopencv_flann \
				-lopencv_ximgproc \
				-lopencv_imgcodecs \
				-lopencv_text \
				-lopencv_videoio

all: 	$(TARGET)


$(TARGET): $(SOURCES)
	g++ $(SOURCES) $(CPPFLAGS) $(INCLUDES) $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)

prepare:
	apt-get update	
	apt-get install --assume-yes g++
	apt-get install --assume-yes libboost-system-dev libboost-filesystem-dev 
	apt-get install --assume-yes libopencv-dev
	apt-get install --assume-yes openalpr openalpr-daemon openalpr-utils libopenalpr-dev

install:
