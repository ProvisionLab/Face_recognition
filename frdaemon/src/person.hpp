#pragma once

#include <string>
#include <cstdint>
#include <map>
#include <chrono>
#include <vector>
#include <list>
#include <memory>
#include <fstream>

#include <opencv2/opencv.hpp>

/// contains samples for recognize person

class DbPersonQuery;
class DbPersonUpdate;

class Person
{
public:

	Person(std::string const & guid, std::string const & features_json);

	void create_features(std::vector<uint8_t> const & fdata);

	void set_features_json(std::string const & json);
	std::string get_features_json() const;

	size_t get_memory_usage();

public:

	std::string	guid;

	std::list<std::vector<float>>	features;

	long version = 0;

	std::chrono::system_clock::time_point	last_recognize_time;	/// time of last successfull recognize
};


/// manages set of person data
class PersonSet
{
public:

	PersonSet();
	~PersonSet();

	void load_from_ftp(std::string const & ftp_url);
	bool load_from_sql(
		std::string const & host, 
		std::string const & db_name, 
		std::string const & db_username, 
		std::string const & db_password,
		std::string const & ftp_url);

	std::vector<std::shared_ptr<Person>> recognize(cv::Mat const & frame);

	std::vector<cv::Mat> load_from_ftp(std::string const & ftp_url, std::string const & person_guid);

public:

	// part of persons which is persistent in memory
	std::list<std::shared_ptr<Person>>	persons;
};
