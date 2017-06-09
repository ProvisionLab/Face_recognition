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
class Person
{
public:

	Person(std::string const & guid)
		: guid(guid)
	{
	}

	/// serialization
	Person(std::ifstream & f);
	void save(std::ofstream & f);

	void append_sample(std::string const & fn, std::vector<uint8_t> data);

	size_t get_memory_usage();

public:

	std::string	guid;

	std::vector<float>	features;

	std::map<std::string, cv::Mat>	files;

	std::chrono::system_clock::time_point	last_recognize_time;	/// time of last successfull recognize
};


/// manages set of person data
class PersonSet
{
public:

	PersonSet();
	~PersonSet();

	void load_from_ftp(std::string const & ftp_url);

	std::vector<std::shared_ptr<Person>> recognize( cv::Mat const & frame );

public:

	bool swap_mode = true;
	std::string		swap_file_name;

	size_t	preload_persons_threshold = 100; /// size of portion which is loaded from disk at a time
	size_t  max_memory_usage = 1000000000; // 1Gb

	// part of persons which is persistent in memory
	std::list<std::shared_ptr<Person>>	persons;
};
