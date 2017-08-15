#pragma once

#ifdef _WIN32
#include <iostream>
#define LOG_ERR		0
#define LOG_WARNING	0
#define LOG_NOTICE	0
#define LOG_INFO	0
#define LOG_DEBUG	0
#else
#endif

#ifdef USE_DAEMON

#define DAEMON_NAME "trdaemon"

#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#endif

inline void sys_log_stream( int priority, std::string const & s )
{
#ifdef _WIN32
	std::cerr << s << std::endl;
#else
	syslog(priority, "%s", s.c_str());
#endif
}

#include <sstream>

#define L_RESULT(x)		(0)
#define LOG(f,x)		sys_log_stream(f, ((std::stringstream&)(std::stringstream() << x)).str())

#else // !USE_DAEMON

#include <iostream>

#define L_RESULT(x)		std::cout << x << std::endl;
#define LOG(f,x)		std::cerr << x << std::endl

#endif // USE_DAEMON
