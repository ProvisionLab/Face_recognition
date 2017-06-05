

redis configuration keys:

any slot contains: 

frd:0 camera		- url of remote camera
frd:0 ftp		- url of ftp storage in form:  "ftp://host/App_Data/Media/"
frd:0 channel		- name of redis channel to publish recognized person's guids
frd:0:lock		- client creates this key to acquire slot. contains client id
