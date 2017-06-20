#include "person.hpp"

#include "ftp_client.hpp"
#include "frame_features.hpp"

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <sstream>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

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

#if 0
		std::list<std::shared_ptr<PersonFeatures>> ls;
		for (auto & p : persons)
			ls.push_back(p);

		ff.compare_persons(ls);

#else	

		ff.compare_persons([this](std::function<void(std::shared_ptr<PersonFeatures>)> action)
		{
			for (auto & person : persons)
			{
				action(person);
			}
		});

#endif
		std::vector<std::shared_ptr<Person>> found;
		for (auto & p : ff.get_found_persons())
		{
			found.push_back(std::static_pointer_cast<Person>(p));
		}

		return found;
	}
	catch (...)
	{
		return{};
	}
}

bool PersonSet::load_from_sql(
	std::string const & host, 
	std::string const & db_name, 
	std::string const & db_username, 
	std::string const & db_password,
	std::string const & ftp_url)
{
	try
	{

		if (!m_sql_conn.connect(host, db_name, db_username, db_password))
			return false;

		FtpClient ftp;
		ftp.ftp_url = ftp_url;

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

						person->append_features(fdata);
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

		// remove persons with invalid key_features
		for (auto i = persons.begin(); i != persons.end();)
		{
			if (*i)
				++i;
			else
				i = persons.erase(i);
		}

//		LOG_DEBUG(persons.size() << " persons loaded\n");

		return true;
	}
	catch ( ODBC::Exception const & e )
	{
		for (auto & m : e.messages)
		{
			LOG_DEBUG(m);
		}

		throw std::runtime_error("sql database");
	}
}

void PersonSet::store_id_to_sql_log(int camera_id, ODBC::UUID person_id)
{
	DbPersonInsertLog q(m_sql_conn);

	q.execute(camera_id, person_id);
}
