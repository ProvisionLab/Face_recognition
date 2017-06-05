#include "redis_client.hpp"

#include <boost/algorithm/string.hpp>
#include <chrono>

std::vector<std::string> RedisClient::parse_line_list(std::string const & list)
{
	std::vector<std::string> subs;
	boost::split(subs, list, [](auto c) { return c == '\n'; });
	return subs;
}

std::map<std::string, std::string> RedisClient::parse_key_list(std::string const & list)
{
	std::vector<std::string> subs;
	boost::split(subs, list, [](auto c) { return c == ' '; });

	std::map<std::string, std::string> values;
	for (auto & p : subs)
	{
		std::vector<std::string> pair;
		boost::split(pair, p, [](auto c) { return c == '='; });

		values[pair[0]] = pair[1];
	}

	return values;
}

int RedisClient::get_client_id()
{
	auto name = "getid_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count() % 0x0FFFFFF);

	std::vector<redis::command>	cmds;
	cmds.push_back(redis::makecmd("CLIENT") << redis::key("SETNAME") << name);
	cmds.push_back(redis::makecmd("CLIENT") << redis::key("LIST"));
	cmds.push_back(redis::makecmd("CLIENT") << redis::key("SETNAME") << std::string());
	redis_.exec_transaction(cmds);

	auto clients = parse_line_list(cmds[1].get_bulk_reply());

	for (auto & c : clients)
	{
		auto values = parse_key_list(c);

		if (values["name"] == name)
		{
			return std::stoi(values["id"]);
		}
	}

	return -1;
}

RedisClient::~RedisClient()
{
	if (!config_key.empty())
	{
		redis_.hdel(config_key, "lock");
	}
}

bool RedisClient::get_configuration()
{
	client_id = get_client_id();

	int camera_id = 0;

	std::string key;
	for (;;++camera_id)
	{
		key = "frd:" + std::to_string(camera_id);

		// check if slot exists
		
		if (!redis_.hexists(key, "camera"))
		{
			config_key.clear();
			return false;
		}

		// try register

		redis::command cmd = redis::makecmd("SET")
			<< redis::key(key + ":lock")
			<< std::to_string(client_id)
			<< "EX" << keep_alive_threshold
			<< "NX";

		redis_.exec(cmd);

		if (cmd.reply_type() == redis::status_code_reply && cmd.get_status_code_reply() == "OK")
		{
			// lock successfull
			config_key = key;
			break;
		}
	}

	config_ftp_url = redis_.hget(config_key, "ftp");
	config_camera_url = redis_.hget(config_key, "camera");
	config_channel = redis_.hget(config_key, "channel");

	return true;
}

void RedisClient::keep_alive()
{
	if (!config_key.empty())
	{
		redis::command cmd = redis::makecmd("SET")
			<< redis::key(config_key + ":lock")
			<< std::to_string(client_id)
			<< "EX" << keep_alive_threshold;
	}
}
