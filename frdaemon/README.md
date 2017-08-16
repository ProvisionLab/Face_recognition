# Description

Deamon for person recognition. Face detection and find matches in givin database.

## Getting started

Deamon get stream from IP-camera, get database with persons from MySQL, process frames by CNN and push results into redis.

### Prerequisites

Deamon uses:
* [OpenCV3.2+](https://github.com/opencv/opencv) for working with camera steam
* [Caffe](https://github.com/BVLC/caffe) framework for card's detection and recognition
* [CUDA](https://developer.nvidia.com/cuda-downloads) if run in GPU mode
* [Redis](https://redis.io/) for communicate with admin's app

### Build

Build only for Ubuntu (Linux)

Use ./build_linux/Makefile

Use CPU_ONLY if you want build without GPU usage

### Configurations

#### CNN config

Deamon use MTCNN and VGG models.

All CNNs model and options store separately. Please ask PM about files.

#### Redis config

redis configuration keys:

any slot contains: 

```
hset frd:0 Camera	<url of remote camera>
hset frd:0 CameraNumber	<id of remote camera>
hset frd:0 Ftp		<url of ftp storage in form:  "ftp://host/App_Data/Media/" >
hset frd:0 Channel	<name of redis channel to publish recognized person's guids>
hset frd:0 DbHost	<mssql server>
hset frd:0 DbName	<mssql database>

frd:0:lock		- client creates this key to acquire slot. contains client id. do not create this key manualy!

```

commands:
```
// status
publish DaemonSystem "{\"Command\":1, \"CameraNumber\": 1, \"Module\":0}"

// config
publish DaemonSystem "{\"Command\":2, \"CameraNumber\": 1, \"Module\":0}"

// start
publish DaemonSystem "{\"Command\":3, \"CameraNumber\": 1, \"Module\":0}"

// stop
publish DaemonSystem "{\"Command\":4, \"CameraNumber\": 1, \"Module\":0}"
```

### How it works

After deamon launched by admin's app he get configs from redis. Then Deamon process database with persons. If solution version the same, Deamom does nothing. If solution version differ or some person not contain person's features Deamon generate new features and update database. After that Deamon start working. Launch stream from CameraUrl and start processing frames. If frame contain face Deamon try find it in person's database. If match found, results push in redis.


### Run

To run Deamon just type in console
```
./build_linux/frd
```

