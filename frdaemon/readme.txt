

redis configuration keys:

any slot contains: 

hset frd:0 camera	<url of remote camera>
hset frd:0 ftp		<url of ftp storage in form:  "ftp://host/App_Data/Media/" >
hset frd:0 channel	<name of redis channel to publish recognized person's guids>
hset frd:0 db_host	<mssql server>
hset frd:0 db_name	<mssql database>

frd:0:lock		- client creates this key to acquire slot. contains client id. do not create this key manualy!



commands:

// status
publish DaemonSystem "{\"Command\":1, \"CameraNumber\": 1, \"Module\":0}"

// config
publish DaemonSystem "{\"Command\":2, \"CameraNumber\": 1, \"Module\":0}"

// start
publish DaemonSystem "{\"Command\":3, \"CameraNumber\": 1, \"Module\":0}"

// stop
publish DaemonSystem "{\"Command\":4, \"CameraNumber\": 1, \"Module\":0}"

