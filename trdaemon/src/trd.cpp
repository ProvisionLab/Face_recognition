
#include "redis_client.hpp"
#include "cards_table.hpp"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <csignal>
#include <queue>

#include "log.h"

#include <opencv2/opencv.hpp>

#define STORE_RECOGNIZED_TO_LOGS 0

std::atomic<bool>	sig_term(false);
std::atomic<bool>	sig_hup(false);
std::atomic<bool>	g_bPause(true);

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

void recognize(CardsTable & table, RedisClient & redis)
{
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

						auto results = table.recognize(frame).to_json();

						if (!results.empty())
						{
							redis.report_recognized(results);
#ifdef _DEBUG
							//std::cout << results << std::endl;
#endif
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
	catch (...)
	{
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

		CardsTable table;

		if (table.set_config(redis.config_table) && table.init())
		{
			LOG(LOG_INFO, "start recognition...");

			recognize(table, redis);
		}
		else
		{
			LOG(LOG_INFO, "invalid table config");
			redis.send_error_status("invalid table config");
		}

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
		std::cerr << "Table recognition daemon" << std::endl;
		std::cerr << "Using: trdaemon <redis_host> [<redis_port>]" << std::endl;
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
			LOG(LOG_ERR, "redis error: " << e.what());
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
