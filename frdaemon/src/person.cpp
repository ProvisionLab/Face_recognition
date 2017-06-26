
#include "person.hpp"

#include "ftp_client.hpp"
#include "frame_features.hpp"

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <sstream>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"

#if !defined(USE_DAEMON) && defined(_DEBUG)

#include <iostream>
#define LOG_DEBUG(x)	std::cerr << x

#else

#define LOG_DEBUG(x)	(0)

#endif

extern std::atomic<bool> sig_term;

Person::Person(unsigned char uuid[16], std::string const & person_desc, std::string const & features_json, long version)
	: person_desc(person_desc)
	, version(version)
{
	memcpy(person_id, uuid, sizeof(person_id));

	if (version == SOLUTION_VERSION)
	{
		set_features_json(features_json);
	}
}

void Person::append_features(std::vector<uint8_t> const & fdata)
{
	cv::Mat image;

	try
	{
		image = cv::imdecode(cv::InputArray(fdata), -1);
		if (image.empty())
			return;
	}
	catch (...)
	{
	}

	append_sample(image);
}

void Person::set_features_json(std::string const & json)
{
	features.clear();

	if (json.empty())
		return;

	try
	{
		rapidjson::Document doc;
		doc.Parse(json.c_str());

		if (!doc.IsArray() || doc.Empty())
			return;

		auto jfss = doc.GetArray();
		for (auto & jfs : jfss)
		{
			std::vector<float> fs;
			for (auto & jf : jfs.GetArray())
				fs.push_back(jf.GetFloat());

			features.push_back(std::move(fs));
		}
	}
	catch (std::exception const &)
	{
	}
}

std::string Person::get_features_json() const
{
	rapidjson::Document doc;

	auto & allocator = doc.GetAllocator();

	doc.SetArray();

	for (auto & fs : features)
	{
		rapidjson::Value jfs(rapidjson::kArrayType);

		for (auto & f : fs)
		{
			jfs.PushBack(f, allocator);
		}

		doc.PushBack(jfs, allocator);
	}

	rapidjson::StringBuffer buffer;
	buffer.Clear();

	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	return std::string(buffer.GetString(), buffer.GetLength());
}

size_t Person::get_memory_usage()
{
	size_t sz = sizeof(Person);

	//sz += guid.size();

	// 2do:
	//sz += features.size() * sizeof(float);

	return sz;
}

PersonSet::PersonSet()
{

}

PersonSet::~PersonSet()
{
}

std::vector<std::shared_ptr<Person>> PersonSet::recognize(cv::Mat const & frame)
{
	try
	{
		FrameFeatures ff;
		ff.generate_features(frame);

		auto found = ff.compare_persons(m_persons_features);

		std::vector<std::shared_ptr<Person>> found2;
		found2.reserve(found.size());

		for (auto & p : found)
		{
			found2.push_back(std::static_pointer_cast<Person>(p));
		}

		return found2;
	}
	catch (...)
	{
		return{};
	}
}

void PersonSet::load_from_sql(RedisClient & redis, std::string const & db_username, std::string const & db_password)
{
	try
	{
		if (!m_sql_conn.connect(redis.config_db_host, redis.config_db_name, db_username, db_password))
			throw std::runtime_error("no sqldb connection");

		FtpClient ftp;
		ftp.ftp_url =redis.config_ftp_url;

		// get person list

		{
			DbPersonQuery query(m_sql_conn);
			while (query.next())
			{
				auto person = std::make_shared<Person>(query.person_id, query.person_desc, query.key_features, query.solution_version);
				persons.push_back(person);
			}
		}

		// update persons samples

		int u_count = 0;

		for (auto & person : persons)
		{
			if (person->version != SOLUTION_VERSION)
			{
				{
					// get sample files
					DbPersonQuerySamples q(m_sql_conn, person->person_id);

					while (q.next())
					{
						auto fdata = ftp.get_file(q.sample_url);

						if (!fdata.empty())
						{
							person->append_features(fdata);
						}
						else
						{
							redis.send_error_status("ftp: get sample '" + q.sample_url + "' for person " + person->person_desc + " failed");
						}
					}

					if (!person->features.empty())
						person->version = SOLUTION_VERSION;
				}

				if (person->version == SOLUTION_VERSION)
				{
					// update database
					DbPersonUpdateSample q(m_sql_conn);
					q.execute(person->person_id, person->get_features_json(), SOLUTION_VERSION);

					LOG_DEBUG("person " << person->person_desc << " samples updated\n");
					++u_count;
				}
				else
				{
					person.reset();
				}
			}
			else
			{
				LOG_DEBUG("person " << person->person_desc << " samples loaded\n");
			}
		}

		LOG_DEBUG(persons.size() << " persons checked\n");
		LOG_DEBUG(u_count << " persons updated\n");

#if TEST_USE_PERSONS_COUNT
		for (int i = 0; i < TEST_USE_PERSONS_COUNT; ++i)
		{
			std::string desc("vp_" + std::to_string(i));
			desc.resize(16, '_');

			ODBC::UUID uuid = {};
			memcpy(uuid, desc.data(), 16);

			auto person = std::make_shared<Person>(uuid, desc, "", 0);
			person->generate_random();
			person->version = SOLUTION_VERSION;

			persons.push_back(person);
		}
#endif

		// remove persons with invalid key_features

		std::vector<std::shared_ptr<PersonFeatures>> ps;

		for (auto i = persons.begin(); i != persons.end();)
		{
			if (*i)
				ps.push_back(*i++);
			else
				i = persons.erase(i);
		}

//		LOG_DEBUG(persons.size() << " persons loaded\n");

		m_persons_features = std::move(PersonFeaturesSet(ps));
	}
	catch ( ODBC::Exception const & e )
	{
		for (auto & m : e.messages)
		{
			LOG_DEBUG(m);
		}

		throw std::runtime_error("sqldb internal error");
	}
}

void PersonSet::store_id_to_sql_log(int camera_id, ODBC::UUID person_id)
{
	DbPersonInsertLog q(m_sql_conn);

	q.execute(camera_id, person_id);
}
