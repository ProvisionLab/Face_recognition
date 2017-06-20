#include "redis_client.hpp"
#include "person.hpp"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <csignal>
#include "frame_features.hpp"
#include "log.h"

#include "recognition/caffe_binding.h"
caffe::CaffeBinding* kCaffeBinding = new caffe::CaffeBinding();



#define STORE_RECOGNIZED_TO_LOGS 0

#ifdef _WIN32
static const std::string config_db_username = "casino_user";
static const std::string config_db_password = "casino";
#else
static const std::string config_db_username = "sa";
static const std::string config_db_password = "Admin123!";
#endif

std::atomic<bool>	sig_term(false);
std::atomic<bool>	sig_hup(false);

const int config_person_recognition_period = 5; // 5 seconds

void recognize(PersonSet & persons, RedisClient & redis)
{
	std::list<PersonSet::PersonPtr>	found;
	std::mutex mx_found;
	std::condition_variable		cv_found;

	volatile bool bThreadError = false;

	std::thread recognoze_thread([&]()
	{
		// open camera
		try
		{
			cv::VideoCapture camera(redis.config_camera_url);

			while (camera.isOpened())
			{
				// get camera frame

				if (sig_term || sig_hup)
					return;

				cv::Mat frame;
				camera >> frame;

				if (!frame.empty())
				{
					// recognize frame using persons

					auto ps = persons.recognize(frame);

					auto time = std::chrono::system_clock::now();

					{
						std::lock_guard<std::mutex> l(mx_found);

						for (auto & person : ps)
						{
							if ((time - person->last_recognize_time) > std::chrono::seconds(config_person_recognition_period))
							{
								person->last_recognize_time = time;

								found.push_back(person);
							}
						}

						if (!found.empty())
							cv_found.notify_one();
					}
				}
				else
				{
					// sleep 1 sec in case of invalid camera capture
					std::this_thread::sleep_for(std::chrono::seconds(1));
				}

				redis.keep_alive();
			}
		}
		catch (...)
		{
			bThreadError = true;
		}
	});

	std::unique_lock<std::mutex> l(mx_found);

	while (!sig_term && !sig_hup && !bThreadError)
	{
		if (found.empty())
		{
			cv_found.wait_for(l, std::chrono::seconds(1));

			if (found.empty())
				continue;
		}

		std::list<PersonSet::PersonPtr> ps;

		ps.splice(ps.end(), found);

		l.unlock();

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

	if (bThreadError)
	{
		LOG(LOG_ERR, "error was ocured while recognize");
	}

	recognoze_thread.join();
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
	try
	{
		RedisClient redis(redis_host, redis_port);

		LOG(LOG_INFO, "reading configuration...");

		if (!redis.get_configuration())
		{
			LOG(LOG_ERR, "there no free slots");
			return;
		}

		LOG(LOG_INFO, "slot id   : " << redis.config_key);
		LOG(LOG_DEBUG, "camera url: " << redis.config_camera_url);
		LOG(LOG_DEBUG, "ftp url   : " << redis.config_ftp_url);
		LOG(LOG_DEBUG, "channel   : " << redis.config_channel);
		LOG(LOG_DEBUG, "db_host   : " << redis.config_db_host);
		LOG(LOG_DEBUG, "db_name   : " << redis.config_db_name);

		LOG(LOG_INFO, "loading of persons samples...");

		PersonSet persons;

		if (!persons.load_from_sql(
			redis.config_db_host, redis.config_db_name,
			config_db_username, config_db_password,
			redis.config_ftp_url))
		{
			LOG(LOG_ERR, "no sql connection");
			return;
		}

		LOG(LOG_INFO, persons.persons.size() << " persons loaded");

		if (persons.persons.empty())
		{
			LOG(LOG_INFO, "no person for recognition");
			return;
		}

		LOG(LOG_INFO, "start recognition...");

		recognize(persons, redis);
	}
	catch (std::exception const & e)
	{
		LOG(LOG_ERR, "error: " << e.what());
	}
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

	FrameFeatures::initialize();

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

		run(redis_host, redis_port);

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
