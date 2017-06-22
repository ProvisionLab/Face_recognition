#pragma once

#include "mssql_client.hpp"
#include "redis_client.hpp"

#include <string>
#include <cstdint>
#include <map>
#include <vector>
#include <list>
#include <chrono>
#include <memory>
#include <fstream>

#include <opencv2/opencv.hpp>

#include "frame_features.hpp"

class RedisClient;

/// contains samples for recognize person

class Person 
	: public PersonFeatures
{
public:

	Person(unsigned char uuid[16], std::string const & person_desc, std::string const & features_json, long version);

	void append_features(std::vector<uint8_t> const & fdata);

	void set_features_json(std::string const & json);
	std::string get_features_json() const;

	size_t get_memory_usage();

public:

	unsigned char	person_id[16];
	std::string		person_desc;

	long version = 0;

	std::chrono::system_clock::time_point	last_recognize_time;	/// time of last successfull recognize
};


/// manages set of person data
class PersonSet
{
public:

	typedef std::shared_ptr<Person>	PersonPtr;

	PersonSet();
	~PersonSet();

	void load_from_sql(RedisClient & redis, std::string const & db_username, std::string const & db_password);

	std::vector<PersonPtr> recognize(cv::Mat const & frame);

	void store_id_to_sql_log(int camera_id, ODBC::UUID person_id);

public:

	std::list<PersonPtr>	persons;

private:

	ODBC::Connection	m_sql_conn;

	PersonFeaturesSet	m_persons_features;
};
