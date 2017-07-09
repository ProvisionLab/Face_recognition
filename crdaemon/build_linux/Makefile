
TARGET = crdaemon

ifdef DEBUG
TARGET = crdaemond
endif

HEADERS = \
	../src/plates_recognition/plates_recognition.hpp \
	../src/plates_recognition/text_detection.hpp

SOURCES = \
	../src/crd.cpp \
	../src/frame.cpp \
	../src/redis_client.cpp \
	../src/redis-cplusplus-client/anet.c \
	../src/plates_recognition/plates_recognition.cpp \
	../src/plates_recognition/text_detection.cpp


CPPFLAGS 	= -std=c++11 -D_DEFAULT_SOURCE 

ifdef DEBUG
CPPFLAGS 	+= -D_DEBUG -Og -ggdb -Wall -g
endif

#CPPFLAGS 	+= -DUSE_DAEMON

INCLUDEPATH += /usr/local/include/opencv


LIBS 		+= -lpthread
LIBS 		+= -lboost_system -lboost_filesystem
LIBS 		+= -L/usr/local/lib \
				-lopencv_core \
				-lopencv_imgproc \
				-lopencv_highgui \
				-lopencv_ml \
				-lopencv_video \
				-lopencv_features2d \
				-lopencv_calib3d \
				-lopencv_objdetect \
				-lopencv_contrib \
				-lopencv_legacy \
				-lopencv_flann \
				-lopencv_ximgproc \
				-lopencv_imgcodecs \
				-lopencv_text \
				-lopencv_videoio

LIBS += -lopenalpr

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