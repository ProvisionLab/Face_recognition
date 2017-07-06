

redis configuration keys:

any slot contains: 

hset crd:0 Camera	<url of remote camera>
hset crd:0 CameraNumber	<id of remote camera>
hset crd:0 Channel	<name of redis channel to publish recognized nums>

crd:0:lock		- client creates this key to acquire slot. contains client id. do not create this key manualy!



commands:

// status
publish DaemonSystem "{\"Command\":1, \"CameraNumber\": 1, \"Module\":1}"

// config
publish DaemonSystem "{\"Command\":2, \"CameraNumber\": 1, \"Module\":1}"

// start
publish DaemonSystem "{\"Command\":3, \"CameraNumber\": 1, \"Module\":1}"

// stop
publish DaemonSystem "{\"Command\":4, \"CameraNumber\": 1, \"Module\":1}"

