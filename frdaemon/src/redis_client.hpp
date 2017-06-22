#pragma once

#include <string>

#include "redis-cplusplus-client/redisclient.h"

#ifdef USE_DAEMON
//#include <assert.h>
//#ifdef assert
//#undef assert
//#endif
//#define assert(x)	if(!x) throw std::runtime_error("assert")
#endif // USE_DAEMON

enum class RedisCommand
{
	Failed			= -1,
	Recognized		= 0,
	Status			= 1,
	ConfigUpdate	= 2
};

class RedisClient
{
public:

	static const int REDIS_PORT = 6379;

	RedisClient(std::string const &host, std::string const & port)
		: redis_(host, port.empty() ? REDIS_PORT : std::stoi(port))
	{
	}

	~RedisClient();

	// get configuration from central db
	// return false if no free slots
	bool get_configuration();

	void person_found(std::string const & person_id)
	{
		send_message(RedisCommand::Recognized, person_id);
	}

	void send_error_status(std::string const & error_message)
	{
		send_message(RedisCommand::Failed, error_message);
	}

	void send_message(RedisCommand command, std::string const & message);

	int get_client_id();

	void keep_alive();

private:

	redis::client	redis_;

	static std::map<std::string, std::string> parse_key_list(std::string const & list);
	static std::vector<std::string> parse_line_list(std::string const & list);

public:

	// configuration

	int client_id = 0;
	int keep_alive_threshold = 60;
	std::string		config_key;
	std::string		config_ftp_url;
	std::string		config_camera_url;
	std::string		config_camera_number;
	std::string		config_channel;

	std::string		config_db_host;
	std::string		config_db_name;
};
