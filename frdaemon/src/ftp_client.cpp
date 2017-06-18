#include "ftp_client.hpp"

#include <iostream>

#define SKIP_PEER_VERIFICATION
#define SKIP_HOSTNAME_VERIFICATION

#include <curl/curl.h>

#include <boost/algorithm/string.hpp>

#include <memory>

#include <atomic>

extern std::atomic<bool> sig_term;

#ifdef _DEBUG
static int debug_callback(CURL *handle,
	curl_infotype type,
	char *data,
	size_t size,
	void *userptr)
{
	std::string s;
	switch (type)
	{
	case CURLINFO_TEXT:
		s.assign(data, size);
		std::cout << "CURL TEXT: " << s << std::endl;
		break;
	case CURLINFO_HEADER_IN:
		s.assign(data, size);
		std::cout << "CURL HEADER_IN: " << s << std::endl;
		break;
	case CURLINFO_HEADER_OUT:
		s.assign(data, size);
		std::cout << "CURL HEADER_OUT: " << s << std::endl;
		break;
	case CURLINFO_DATA_IN:
		s.assign(data, size);
		std::cout << "CURL DATA_IN: " << s << std::endl;
		break;
	case CURLINFO_DATA_OUT:
		s.assign(data, size);
		std::cout << "CURL DATA_OUT: " << s << std::endl;
		break;
	default:
		break;
	}

	return 0;
}
#endif // DEBUG

size_t FtpClient::write_file_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
	auto * data = static_cast<std::vector<uint8_t>*>(userp);

	auto len = size*nmemb;

	if (data)
		data->insert(data->end(), (uint8_t const*)ptr, (uint8_t const*)ptr+len);

	return len;
}

size_t FtpClient::write_list_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
	auto * slist = static_cast<std::string*>(userp);

	auto len = size*nmemb;

	std::string str((char*)ptr, len);

	if (slist)
		slist->append(str);

	return len;
}

void FtpClient::init(void * curl)
{
	bool use_tls = boost::starts_with(ftp_url, "ftps://");

	if (use_tls)
	{
		curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

#ifdef SKIP_PEER_VERIFICATION
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
#ifdef SKIP_HOSTNAME_VERIFICATION
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif
	}

	if (!username.empty())
		curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());

	if (!password.empty())
		curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());

#ifdef _DEBUG
	if (verbose)
	{
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, &debug_callback);
	}
#endif
}

std::vector<std::string> FtpClient::get_list(std::string dirname)
{
	if (ftp_url.empty())
		throw std::invalid_argument(__FUNCTION__);

	// get a curl handle
	std::unique_ptr<CURL, void(*)(CURL*)> curl_ptr(curl_easy_init(), &curl_easy_cleanup);
	CURL * curl = curl_ptr.get();

	CURLcode res = CURLE_OK;

	if (!curl)
		throw std::runtime_error(__FUNCTION__);

	init(curl);

	std::string str;

	// we want to use our own write function
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_list_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &str);
	curl_easy_setopt(curl, CURLOPT_DIRLISTONLY, 1);

	// specify target
	std::string remote_url(ftp_url);

	if (remote_url.back() != '/')
		remote_url += '/';

	remote_url += dirname;

	if (remote_url.back() != '/')
		remote_url += '/';

	curl_easy_setopt(curl, CURLOPT_URL, remote_url.c_str());

	// Now run off and do what you've been told!
	res = curl_easy_perform(curl);

	// Check for errors
	if (res != CURLE_OK)
	{
		//std::cerr << __FUNCTION__ << ", url: " << remote_url << std::endl;
		//std::cerr << __FUNCTION__ << ", curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;

		return {};
	}
	else
	{
		std::vector<std::string> list;

		std::vector<std::string> lines;
#ifdef _WIN32
		boost::iter_split(lines, str, boost::algorithm::first_finder("\r\n"));
#else
		boost::iter_split(lines, str, boost::algorithm::first_finder("\n"));
#endif

		for (auto & line : lines)
		{
			if (!line.empty())
			{
				list.emplace_back(std::move(line));
			}
		}

		return list;
	}

}

std::vector<uint8_t> FtpClient::get_file(std::string filepath)
{
	if (ftp_url.empty())
		throw std::invalid_argument(__FUNCTION__);

	// get a curl handle
	std::unique_ptr<CURL, void(*)(CURL*)> curl_ptr(curl_easy_init(), &curl_easy_cleanup);
	CURL * curl = curl_ptr.get();

	CURLcode res = CURLE_OK;

	if (!curl)
		throw std::runtime_error(__FUNCTION__);

	init(curl);

	std::vector<uint8_t>	data;

	// we want to use our own write function
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_file_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

	// specify target
	std::string remote_url(ftp_url);

	if (remote_url.back() != '/')
		remote_url += '/';

	remote_url += filepath;

	curl_easy_setopt(curl, CURLOPT_URL, remote_url.c_str());

	// Now run off and do what you've been told!
	res = curl_easy_perform(curl);

	// Check for errors
	if (res != CURLE_OK)
	{
		//std::cerr << __FUNCTION__ << ", url: " << remote_url << std::endl;
		//std::cerr << __FUNCTION__ << ", curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;

		return {};
	}
	else
	{
		return data;
	}
}

std::vector<std::vector<uint8_t>> FtpClient::get_files(std::string dirpath)
{
	std::vector<std::vector<uint8_t>> files;

	auto file_names = get_list(dirpath);

	for (auto & fn : file_names)
	{
		if (sig_term)
			return{};

		files.emplace_back(get_file(dirpath + "/" + fn));
	}

	return std::move(files);
}
