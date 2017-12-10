
TARGET = trdaemon

ifdef DEBUG
TARGET = trdaemond
endif

SOURCES = \
	../src/trd.cpp \
	../src/cards_table.cpp \
	../src/redis_client.cpp \
	../src/redis-cplusplus-client/anet.c \
        ../src/cards_recognition/baccarat.cpp \
        ../src/cards_recognition/black_jack.cpp \
        ../src/cards_recognition/cluster.cpp \
        ../src/cards_recognition/convolution_detector.cpp \
        ../src/cards_recognition/kmeans.cpp \
        ../src/cards_recognition/point.cpp \



CPPFLAGS 	= -std=c++14 -D_DEFAULT_SOURCE -DUSE_OPENCV

ifdef DEBUG
CPPFLAGS 	+= -D_DEBUG -Og -ggdb -Wall -g
endif

ifdef CPU_ONLY
CPPFLAGS += -DCPU_ONLY
endif

#CPPFLAGS 	+= -DUSE_DAEMON

LIBS 		+= -lpthread
LIBS 		+= -lboost_system -lboost_filesystem -lboost_thread
LIBS 		+= -lopencv_core -lopencv_imgproc -lopencv_videoio -lopencv_highgui -lopencv_imgcodecs
#LIBS            += -luuid
LIBS            += -lglog

LIBS            += -L/home/cnn2/libs/caffe/build/install/lib -lcaffe

INCLUDES        += -I/home/cnn2/libs/caffe/build/install/include

ifndef CPU_ONLY
LIBS            += -L/usr/local/cuda/lib64 -lcudart -lcublas -lcurand
INCLUDES        += -I/usr/local/cuda/include
endif

all: 	$(TARGET)


$(TARGET): $(SOURCES)
	g++ $(SOURCES) $(CPPFLAGS) $(INCLUDES) $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)

prepare:
	apt-get update	
	apt-get install -y g++
	apt-get install -y libboost-system-dev
	apt-get install -y libopencv-dev

install:
