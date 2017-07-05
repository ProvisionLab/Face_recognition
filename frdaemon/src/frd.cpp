
#include "redis_client.hpp"
#include "person.hpp"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <csignal>
#include <queue>

#include "log.h"

#define STORE_RECOGNIZED_TO_LOGS 0

#define USE_RECOGNIZE_IN_MAIN_THREAD 1

#ifdef _WIN32
static const std::string config_db_username = "casino_user";
static const std::string config_db_password = "casino";
#else
static const std::string config_db_username = "sa";
static const std::string config_db_password = "Admin123!";
#endif

std::atomic<bool>	sig_term(false);
std::atomic<bool>	sig_hup(false);
std::atomic<bool>	g_bPause(true);

const int config_person_recognition_period = 5; // 5 seconds


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

std::list<PersonSet::PersonPtr> filter_found_persons(std::vector<PersonSet::PersonPtr> persons)
{
	std::list<PersonSet::PersonPtr> filtered;

	auto time = std::chrono::system_clock::now();

	for (auto & person : persons)
	{
		if ((time - person->last_recognize_time) > std::chrono::seconds(config_person_recognition_period))
		{
			person->last_recognize_time = time;

			filtered.push_back(person);
		}
	}

	return filtered;
}

void recognize(PersonSet & persons, RedisClient & redis)
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


#if USE_RECOGNIZE_IN_MAIN_THREAD

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

						auto found_persons = filter_found_persons(persons.recognize(frame));

						for (auto & person : found_persons)
						{
							redis.person_found(person->person_desc);
							std::cout << person->person_desc << std::endl;
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

#else // !USE_RECOGNIZE_IN_MAIN_THREAD

	std::list<PersonSet::PersonPtr>	found_persons;

	std::thread recognoze_thread([&]()
	{
		try
		{
			while (!sig_term && !sig_hup)
			{
				if (bPause)
				{
					std::this_thread::sleep_for(std::chrono::seconds(2));
#ifdef _DEBUG
					std::cout << "pause" << std::endl;
#endif
					redis.keep_lock();
				}
				else
				{
					// open camera
					cv::VideoCapture camera(redis.config_camera_url);

					while (camera.isOpened())
					{
						// get camera frame

						if (sig_term || sig_hup)
							return;

						if (bPause)
							break;

						cv::Mat frame;
						camera >> frame;

						if (!frame.empty())
						{
							// recognize frame using persons

							auto ps = persons.recognize(frame);

							{
								std::lock_guard<std::mutex> l(mx_found);

								found_persons = filter_found_persons(ps);

								if (!found_persons.empty())
									cv_found.notify_one();
							}
						}
						else
						{
							// sleep 1 sec in case of invalid camera capture
							std::this_thread::sleep_for(std::chrono::seconds(1));
						}

						redis.keep_lock();
					} // while(camera.isOpened())

				}

			} // while

		}
		catch (...)
		{
			bThreadError = true;
		}
	});

	try
	{
		std::unique_lock<std::mutex> l(mx_found);

		while (!sig_term && !sig_hup && !bThreadError)
		{
			if (incoming_commands.empty() && found_persons.empty())
			{
				cv_found.wait_for(l, std::chrono::seconds(1));
				continue;
			}

			std::queue<RedisCommand> cmds;
			cmds.swap(incoming_commands);

			std::list<PersonSet::PersonPtr> ps;
			ps.swap(found_persons);

			l.unlock();

			// process commands
			process_commands(cmds, redis);

			// process persons

			for (auto & person : ps)
			{
				redis.person_found(person->person_desc);

#if STORE_RECOGNIZED_TO_LOGS
				persons.store_id_to_sql_log(0, person->person_id);
#endif
				std::cout << person->person_desc << std::endl;
			}

			l.lock();
		}
	}
	catch (...)
	{
	}

	recognoze_thread.join();

#endif // USE_RECOGNIZE_IN_MAIN_THREAD

	sig_hup = true;
	redis.listen_sub_stop();

	if (bThreadError)
	{
		LOG(LOG_ERR, "error was ocured while recognize");
	}

	listen_thread.join();
}

#ifdef USE_DAEMON
void daemonize()
{
#ifndef _WIN32

	setlogmask(LOG_UPTO(LOG_INFO));
	openlog(DAEMON_NAME, LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_USER);
//	openlog(DAEMON_NAME, LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_USER);

	LOG(LOG_INFO, "Entering Daemon");

	pid_t pid, sid;
	//Fork the Parent Process
	pid = fork();

	if (pid < 0) { exit(EXIT_FAILURE); }

	//We got a good pid, Close the Parent Process
	if (pid > 0) { exit(EXIT_SUCCESS); }

	//Change File Mask
	umask(0);

	//Create a new Signature Id for our child
	sid = setsid();
	if (sid < 0) { exit(EXIT_FAILURE); }

	//Change Directory
	//If we cant find the directory we exit with failure.
	if ((chdir("/")) < 0) { exit(EXIT_FAILURE); }

	//Close Standard File Descriptors
	//close(STDIN_FILENO);
	//freopen("/dev/null", "r", stdin);
	//close(STDOUT_FILENO);
	//freopen("/dev/null", "a+", stdout);
	//close(STDERR_FILENO);
	//freopen("/dev/null", "a+", stderr);
#endif // _WIN32
}
#endif // USE_DAEMON

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
		LOG(LOG_DEBUG, "ftp url   : " << redis.config_ftp_url);
		LOG(LOG_DEBUG, "report channel : " << redis.config_report_channel);
		LOG(LOG_DEBUG, "listen channel : " << redis.config_listen_channel);
		LOG(LOG_DEBUG, "db_host   : " << redis.config_db_host);
		LOG(LOG_DEBUG, "db_name   : " << redis.config_db_name);

		LOG(LOG_INFO, "loading of persons samples...");

		PersonSet persons;

		persons.load_from_sql(redis, config_db_username, config_db_password);
	
		LOG(LOG_INFO, persons.persons.size() << " persons loaded");

		if (!persons.persons.empty())
		{
			LOG(LOG_INFO, "start recognition...");

			recognize(persons, redis);
		}
		else
		{
			LOG(LOG_INFO, "no person for recognition");
			redis.send_error_status("no person for recognition");
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
#ifndef USE_DAEMON
		std::cerr << "Using: frd <redis_host> [<redis_port>]" << std::endl;
#endif
		return 0;
	}

#ifdef USE_DAEMON

	daemonize();

#endif

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

#if defined(USE_DAEMON) && !defined(_WIN32)
	closelog();
#endif

	return 0;
}
