
#include "redis_client.hpp"
#include "frame.hpp"
#include <opencv2/opencv.hpp>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <csignal>
#include <queue>

#include "log.h"

std::atomic<bool>	sig_term(false);
std::atomic<bool>	sig_hup(false);
std::atomic<bool>	g_bPause(true);

const int config_recognition_period = 1; // 1 seconds


std::string encode_base64(cv::Mat const &img)
{
	std::vector<uint8_t> buff;
	std::vector<int> param = { cv::IMWRITE_JPEG_QUALITY , 80 };
	cv::imencode(".jpg", img, buff, param);

	static const std::string base64_chars =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

	unsigned char const* bytes_to_encode = buff.data();
	unsigned int in_len = (unsigned int)buff.size();

	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (in_len--) {
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; (i <4); i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while ((i++ < 3))
			ret += '=';

	}

	return ret;
}

void process_commands(std::queue<RedisCommand> & commands, RedisClient & redis)
{
	// process commands
	while (!commands.empty())
	{
		auto cmd = commands.front();
		commands.pop();

		switch (cmd)
		{
		case RedisCommand::ConfigUpdate:
			std::cout << "command: ConfigUpdate" << std::endl;
			sig_hup = true;
			break;

		case RedisCommand::Start:
			std::cout << "command: Start" << std::endl;
			g_bPause = false;
			break;

		case RedisCommand::Stop:
			std::cout << "command: Stop" << std::endl;
			g_bPause = true;
			break;

		case RedisCommand::Status:
			std::cout << "command: Status" << std::endl;
			redis.send_status(!g_bPause);
			break;

		default:
			std::cout << "command: " << (int)cmd << std::endl;
		}
	}
}

std::vector<std::pair<std::string, cv::Mat>> filter_results(
	std::vector<std::pair<std::string, cv::Mat>> const & results,
	std::map<std::string, std::chrono::system_clock::time_point> & plates)
{
	std::vector<std::pair<std::string, cv::Mat>> filtered;

	auto now = std::chrono::system_clock::now();

	static const auto delay = std::chrono::seconds(config_recognition_period);

	for (auto & res : results)
	{
		auto & last_time = plates[res.first];
		if ((now - last_time) > delay)
		{
			last_time = now;
			filtered.push_back(res);
		}
	}

	return filtered;
}

void recognize(RedisClient & redis)
{
	init_alpr();

	std::mutex mx_found;
	std::condition_variable		cv_found;

	volatile bool bThreadError = false;

	std::queue<RedisCommand>	incoming_commands;

#ifdef _DEBUG
	g_bPause = false;
#endif

	std::thread listen_thread([&]() 
	{
		try
		{
			redis.listen_sub([&](RedisCommand cmd)
			{
				std::lock_guard<std::mutex> l(mx_found);

				incoming_commands.push(cmd);

				cv_found.notify_one();
			});
		}
		catch (...)
		{
			bThreadError = true;
		}
	});

	try
	{
		std::map<std::string, std::chrono::system_clock::time_point>	plates;

		while (!sig_term && !sig_hup)
		{
			if (g_bPause)
			{
				while (g_bPause && !sig_term && !sig_hup)
				{
					std::queue<RedisCommand> cmds;
					{
						std::unique_lock<std::mutex> l(mx_found);
						cmds.swap(incoming_commands);
					}

					process_commands(cmds, redis);

					std::this_thread::sleep_for(std::chrono::seconds(1));
					redis.keep_lock();
				}
			}
			else 
			{
				// open camera
				cv::VideoCapture camera(redis.config_camera_url);
				while (!g_bPause && !sig_term && !sig_hup && camera.isOpened())
				{
					std::queue<RedisCommand> cmds;
					{
						std::unique_lock<std::mutex> l(mx_found);
						cmds.swap(incoming_commands);
					}

					process_commands(cmds, redis);

					// get camera frame

					cv::Mat frame;
					camera >> frame;

					if (!frame.empty())
					{
						// recognize frame using persons

						auto found = filter_results(recognize_on_frame(frame), plates);

						for (auto & v : found)
						{
							redis.report_recognized(v.first, encode_base64(v.second));
							std::cout << v.first << std::endl;
						}
					}
					else
					{
						// sleep 1 sec in case of invalid camera capture
						std::this_thread::sleep_for(std::chrono::seconds(1));
					}

					redis.keep_lock();

				} // while

			} 

		} // while
	}
	catch (std::exception const & e)
	{
		std::cerr << e.what() << std::endl;
	}

	sig_hup = true;
	redis.listen_sub_stop();

	if (bThreadError)
	{
		LOG(LOG_ERR, "error was ocured while recognize");
	}

	listen_thread.join();
}

void run(std::string const & redis_host, std::string const & redis_port)
{
	RedisClient redis(redis_host, redis_port);

	LOG(LOG_INFO, "reading configuration...");

	try
	{
		if (!redis.get_configuration())
		{
			LOG(LOG_ERR, "there no free slots");
			return;
		}

		LOG(LOG_INFO, "slot id   : " << redis.config_key);
		LOG(LOG_DEBUG, "camera url: " << redis.config_camera_url);
		LOG(LOG_DEBUG, "camera num: " << redis.config_camera_number);
		LOG(LOG_DEBUG, "report channel : " << redis.config_report_channel);
		LOG(LOG_DEBUG, "listen channel : " << redis.config_listen_channel);

		LOG(LOG_INFO, "start recognition...");

		recognize(redis);
	}
	catch (std::exception const & e)
	{
		LOG(LOG_ERR, "error: " << e.what());
		redis.send_error_status(e.what());
	}

	redis.unlock_slot();
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::cerr << "Using: crdaemon <redis_host> [<redis_port>]" << std::endl;
		return 0;
	}

	std::string redis_host(argv[1]);
	std::string redis_port = std::to_string(RedisClient::REDIS_PORT);

	if (argc > 2)
	{
		redis_port = argv[2];
	}

	std::signal(SIGINT, [](int)
	{
		// Ctrl-C
		sig_term = true;
	});

	std::signal(SIGTERM, [](int)
	{
		sig_term = true;
	});
	
#ifdef USE_DAEMON
#ifndef _WIN32

	std::signal(SIGHUP, [](int)
	{
		sig_hup = true;
	});

	std::signal(SIGCHLD, SIG_IGN);

#endif
#endif

	while (true)
	{
		sig_hup = false;

		try
		{
			run(redis_host, redis_port);
		}
		catch (std::exception &e)
		{
			LOG(LOG_ERR, "error: " << e.what());
		}

		// exit from run on error or sig_term or sig_hup

		if (sig_term)
		{
			LOG(LOG_INFO, "stop");
			break;
		}

		if (sig_hup)
		{
			LOG(LOG_INFO, "restart");
			continue;
		}

#ifdef _DEBUG
		// do not run eternally while debug
		LOG(LOG_DEBUG, "exit from DEBUG version");
		break;
#endif

		// on any error sleep for 1 minute & try again
		LOG(LOG_INFO, "waiting for 1 minute...");
		std::this_thread::sleep_for(std::chrono::seconds(60));
	} // while

	return 0;
}
