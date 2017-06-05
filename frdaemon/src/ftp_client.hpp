#pragma once

#include <string>
#include <vector>
#include <cstdint>

class FtpClient
{
public:

	std::vector<std::string> get_list(std::string dirpath);
	std::vector<uint8_t> get_file(std::string filepath);

public:

	std::string ftp_url;
	std::string username;
	std::string password;

	bool verbose = false;

private:

	void init(void * curl);

	static size_t write_file_callback(void *ptr, size_t size, size_t nmemb, void *userp);
	static size_t write_list_callback(void *ptr, size_t size, size_t nmemb, void *userp);
};

