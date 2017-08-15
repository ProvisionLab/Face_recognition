

redis configuration keys:

any slot contains: 

hset trd:0 Camera	<url of remote camera>
hset trd:0 CameraNumber	<id of remote camera>
hset trd:0 Channel	<name of redis channel to publish recognized cards>
hset trd:0 Config	<json string of table config>

trd:0:lock		- client creates this key to acquire slot. contains client id. do not create this key manualy!


commands:

// status
publish DaemonSystem "{\"Command\":1, \"CameraNumber\": 1, \"Module\":2}"

// config
publish DaemonSystem "{\"Command\":2, \"CameraNumber\": 1, \"Module\":2}"

// start
publish DaemonSystem "{\"Command\":3, \"CameraNumber\": 1, \"Module\":2}"

// stop
publish DaemonSystem "{\"Command\":4, \"CameraNumber\": 1, \"Module\":2}"


