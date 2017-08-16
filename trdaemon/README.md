# Description

Deamon for cards detection and recognition on game table. Also available chips detection if playing "baccarat" and split detection if playing "black-jack".

## Getting started

One deamon instance matches one table. Deamon get stream from IP-camera, process by CNN and push results into redis.

### Prerequisites

Deamon uses:
* [OpenCV3.2+](https://github.com/opencv/opencv) for working with camera steam
* [Caffe](https://github.com/BVLC/caffe) framework for card's detection and recognition
* [CUDA](https://developer.nvidia.com/cuda-downloads) if run in GPU mode
* [Redis](https://redis.io/) for communicate with admin's app

### Build

For build under WIN you should use ../../frdeamon.sln

In that solution build project windows/trd

Building configuration - Release x64

For easier setup dependencies for Caffe try to use src/cards_recognition/*.props 

### Configurations

#### Admin's app's config

Example of configuration generatet by admin's app:
```
{
   "CameraUrl":"http://**.**.**.**/axis-cgi/mjpg/video.cgi?x.mjpeg",
   "TableNumber":1,
   "SourceType":1,
   "Game":"baccarat",
   "Crops":[
      {
         "AreaNumber":0,
         "Left":0.37,
         "Top":0.686,
         "Right":0.449,
         "Bottom":0.147
      },
      {
         "AreaNumber":1,
         "Left":0.14,
         "Top":0.578,
         "Right":0.674,
         "Bottom":0.248
      }
   ],
   "Buttons":[
      {
         "AreaNumber":1,
         "Left":0.223,
         "Top":0.265,
         "Right":0.587,
         "Bottom":0.426
      }
   ]
}
```

#### CNN config

example on config for CNN

cascade1-model-structure.prototxt
cascade1-model.caffemodel
cascade2-model-structure.prototxt
cascade2-model.caffemodel
ranks-model-structure.prototxt
ranks-model.caffemodel
suits-model-structure.prototxt
suits-model.caffemodel
image scaling
threshold cascade1 cnn
threshold cascade2 cnn
threshold ranks cnn
threshold suits cnn
image binarization threshold

```
CASCADE1-model_memory_jp.prototxt
cascade1-jumbo_iter_2000.caffemodel
cascade2-model.prototxt
cascade2-jumbo_iter_5000.caffemodel
RANKS-model_jp.prototxt
ranks-jumbo_iter_30000.caffemodel
suits-model_jp.prototxt
suits-jumbo_iter_20000.caffemodel
0.1
0.75
0.99
0.0
0.0
1.0
170
```

#### Redis config

redis configuration keys:

any slot contains: 

```
hset trd:0 Camera	<url of remote camera>
hset trd:0 CameraNumber	<id of remote camera>
hset trd:0 Channel	<name of redis channel to publish recognized cards>
hset trd:0 Config	<json string of table config>

trd:0:lock		- client creates this key to acquire slot. contains client id. do not create this key manualy!
```

commands:
```
// status
publish DaemonSystem "{\"Command\":1, \"CameraNumber\": 1, \"Module\":2}"

// config
publish DaemonSystem "{\"Command\":2, \"CameraNumber\": 1, \"Module\":2}"

// start
publish DaemonSystem "{\"Command\":3, \"CameraNumber\": 1, \"Module\":2}"

// stop
publish DaemonSystem "{\"Command\":4, \"CameraNumber\": 1, \"Module\":2}"
```

### How it works

After deamon launched by admin's app he get configs from redis. Deamon create own virtual table with card's zones and buttons zones (if game "baccarat"). Deamon getting frame from CameraUrl, cut for zones and process this zones separately. If it's card's zone Deamon detecting cards, then recognizing them. If game "black-jack" also checking if split played. If it's buttons zone Deamon just tracking if button placed on this zone. After that results collection and pushing in redis, where logic server could process them.


### Run

Deamon is started directly by the admin's application. If you want to manual run just try in console
```
trd.exe
```
