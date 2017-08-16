# Description

Deamon for car's plate recognition. Plate detection and recognition.

## Getting started

Deamon get stream from IP-camera, process frames by CNN and push results into redis.

### Prerequisites

Deamon uses:
* [OpenCV3.2+](https://github.com/opencv/opencv) for working with camera steam
* [Opencv_contrib](https://github.com/opencv/opencv_contrib) for processing frames
* [OpenALPR](https://github.com/openalpr/openalpr) for plates recognition
* [Redis](https://redis.io/) for communicate with admin's app

### Build

Build only for Ubuntu (Linux)

Use ./build_linux/Makefile

### Configurations

#### Redis config

redis configuration keys:

any slot contains: 

```
hset crd:0 Camera	<url of remote camera>
hset crd:0 CameraNumber	<id of remote camera>
hset crd:0 Channel	<name of redis channel to publish recognized nums>

crd:0:lock		- client creates this key to acquire slot. contains client id. do not create this key manualy!
```

commands:
```
// status
publish DaemonSystem "{\"Command\":1, \"CameraNumber\": 1, \"Module\":1}"

// config
publish DaemonSystem "{\"Command\":2, \"CameraNumber\": 1, \"Module\":1}"

// start
publish DaemonSystem "{\"Command\":3, \"CameraNumber\": 1, \"Module\":1}"

// stop
publish DaemonSystem "{\"Command\":4, \"CameraNumber\": 1, \"Module\":1}"
```

### How it works
Deamon launch manualy. When admin's app start plate's recognition Demon wake up. Deamon launch stream from CameraUrl and start processing frames. First Deamon process frame for plates. If plates finded starting recognition and push results into redis.


### Run

To run Deamon just type in console
```
./build_linux/crd
```
