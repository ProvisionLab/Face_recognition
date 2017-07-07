#pragma once

#include <string>
#include <functional>
#include <chrono>
#include <map>
#include <atomic>

#include "redis-cplusplus-client/redisclient.h"

#ifdef USE_DAEMON
//#include <assert.h>
//#ifdef assert
//#undef assert
//#endif
//#define assert(x)	if(!x) throw std::runtime_error("assert")
#endif // USE_DAEMON

#define MODULE_NAMESPACE		"crd"
#define MODULE_TYPE				ModuleType::Cards

enum class RedisCommand
{
	Failed			= -1,
	Recognized		= 0,
	Status			= 1,
	ConfigUpdate	= 2,
	Start			= 3,
	Stop			= 4
};

enum class ModuleType
{
	Face = 0,
	Cards = 1,
};

class RedisClient
{
public:

	static const int REDIS_PORT = 6379;

	RedisClient(std::string const &host, std::string const & port)
		: host_(host)
		, port_(port.empty() ? REDIS_PORT : std::stoi(port))
		, redis_(host_, port_)
		, m_listen_socket(-1)
	{
	}

	~RedisClient();

	// get configuration from central db
	// return false if no free slots
	bool get_configuration();

	void report_recognized(std::string const & person_id)
	{
		send_message(RedisCommand::Recognized, person_id);
	}

	void send_error_status(std::string const & error_message)
	{
		send_message(RedisCommand::Failed, error_message);
	}

	void send_status(bool status);

	int get_client_id();

	void keep_lock();
	void unlock_slot();

	void listen_sub(std::function<void(RedisCommand)> on_command);
	void listen_sub_stop();

private:

	std::string		host_;
	int				port_;
	redis::client	redis_;

	std::chrono::system_clock::time_point	m_last_keeplock;

	std::atomic<int>	m_listen_socket;

	static std::map<std::string, std::string> parse_key_list(std::string const & list);
	static std::vector<std::string> parse_line_list(std::string const & list);

	void send_message(RedisCommand command, std::string const & message);

public:

	// configuration

	int client_id = 0;
	int lock_lifetime	= 60;
	int relock_period	= 5;

	std::string		config_key;
	std::string		config_camera_url;
	std::string		config_camera_number;

	std::string		config_report_channel;
	std::string		config_listen_channel = "DaemonSystem";
};
