#include "redis_client.hpp"

#include <boost/algorithm/string.hpp>
#include <chrono>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"

std::vector<std::string> RedisClient::parse_line_list(std::string const & list)
{
	std::vector<std::string> subs;
	boost::split(subs, list, [](char c) { return c == '\n'; });
	return subs;
}

std::map<std::string, std::string> RedisClient::parse_key_list(std::string const & list)
{
	std::vector<std::string> subs;
	boost::split(subs, list, [](char c) { return c == ' '; });

	std::map<std::string, std::string> values;
	for (auto & p : subs)
	{
		std::vector<std::string> pair;
		boost::split(pair, p, [](char c) { return c == '='; });

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
		
		if (!redis_.hexists(key, "Camera"))
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

	config_ftp_url			= redis_.hget(config_key, "Ftp");
	config_camera_url		= redis_.hget(config_key, "Camera");
	config_camera_number	= redis_.hget(config_key, "CameraNumber");
	config_report_channel	= redis_.hget(config_key, "Channel");
	//config_listen_channel	= redis_.hget(config_key, "ListenChannel");
	config_db_host			= redis_.hget(config_key, "DbHost");
	config_db_name			= redis_.hget(config_key, "DbName");

	return true;
}

void RedisClient::keep_alive()
{
	if (!config_key.empty())
	{
		auto now_ = std::chrono::system_clock::now();

		if ((now_ - m_last_keepalive) >= std::chrono::seconds(keep_alive_period))
		{
			redis::command cmd = redis::makecmd("SET")
				<< redis::key(config_key + ":lock")
				<< std::to_string(client_id)
				<< "EX" << keep_alive_threshold;

			m_last_keepalive = now_;
		}
	}
}

void RedisClient::send_message(RedisCommand command, std::string const & message)
{
	rapidjson::Document doc;

	auto & allocator = doc.GetAllocator();

	doc.SetObject();
	doc.AddMember("Command", (int)command, allocator);
	doc.AddMember("CameraNumber", std::stoi(config_camera_number), allocator);
	doc.AddMember("Data", rapidjson::StringRef(message.data(), message.size()), allocator);

	rapidjson::StringBuffer buffer;
	buffer.Clear();

	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	std::string json(std::string(buffer.GetString(), buffer.GetLength()));
	redis_.publish(config_report_channel, json);
}

void RedisClient::listen_sub(std::function<void(RedisCommand)> on_command)
{
	redis::client	lr(host_, port_);

	m_listen_socket = lr.get_socket(config_listen_channel);

	lr.subscribe(config_listen_channel, [this, on_command](std::string const & message)
	{
		try
		{
			rapidjson::Document doc;
			doc.Parse(message.c_str());

			if (doc.HasMember("Module"))
			{
				auto module_id = static_cast<ModuleType>(doc["Module"].GetInt());
				if (module_id != ModuleType::Face)
					return;
			}

			auto command = static_cast<RedisCommand>(doc["Command"].GetInt());

			if (doc.HasMember("CameraNumber"))
			{
				auto camera_id = doc["CameraNumber"].GetInt();
				if (camera_id >= 0 && camera_id != std::stoi(config_camera_number))
					return;
			}

			on_command(command);
		}
		catch (...)
		{
			std::cerr << "invalid message from channel " << config_listen_channel << ": " << message << std::endl;
		}
	});

	m_listen_socket = -1;
}

void RedisClient::listen_sub_stop()
{
	// 2do:
	int socket = m_listen_socket;
	redis::client::stop_subscribe(socket);
}
