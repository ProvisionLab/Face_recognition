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

Person::Person(unsigned char uuid[16], std::string const & sample_url, std::string const & features_json)
	: sample_url(sample_url)
{
	memcpy(person_id, uuid, sizeof(person_id));
	set_features_json(features_json);
}

void Person::create_features(std::vector<uint8_t> const & fdata)
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

	features.emplace_back( generate_features_for_sample(image) );
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

std::vector<cv::Mat> PersonSet::load_from_ftp(std::string const & ftp_url, std::string const & person_guid)
{
	FtpClient ftp;
	ftp.ftp_url = ftp_url;

	auto files = ftp.get_list(person_guid);

	std::vector<cv::Mat> images;

	for (auto & fn : files)
	{
		LOG_DEBUG("loading file: " << fn);
		auto fdata = ftp.get_file(person_guid + "/" + fn);

		if (!fdata.empty())
		{
			LOG_DEBUG(" ok, size: " << fdata.size() << std::endl);

			try
			{
				cv::Mat image = cv::imdecode(cv::InputArray(fdata), -1);

				if (!image.empty())
				{
					images.emplace_back(image);
				}

			}
			catch (...)
			{
			}

		}
		else
		{
			LOG_DEBUG(" fail" << std::endl);
		}
	}

	return std::move(images);
}

void PersonSet::load_from_ftp(std::string const & ftp_url)
{
	FtpClient ftp;
	ftp.ftp_url = ftp_url;

	auto persone_list = ftp.get_list("");

	size_t memory_usage = 0;

	for (auto & guid : persone_list)
	{
		LOG_DEBUG("person guid: " << guid << std::endl);

		Person person({}, guid, "");

		auto files = ftp.get_list(guid);
		for (auto & fn : files)
		{
			if (sig_term)
				return;

			LOG_DEBUG("loading file: " << fn);
			auto fdata = ftp.get_file(guid + "/" + fn);

			if (!fdata.empty())
			{
				LOG_DEBUG(" ok, size: " << fdata.size() << std::endl);
				person.create_features(fdata);
			}
			else
			{
				LOG_DEBUG(" fail" << std::endl);
			}
		}

		if (!person.features.empty())
		{
			memory_usage += person.get_memory_usage();

			persons.push_back(std::make_shared<Person>(std::move(person)));
		}
	}
}

std::vector<std::shared_ptr<Person>> PersonSet::recognize(cv::Mat const & frame)
{
	std::vector<std::shared_ptr<Person>> found;

	FrameFeatures ff;
	ff.generate_features(frame);

	for (auto & person : persons)
	{
		if (sig_term)
			return{};

		if (ff.contains_person(person->features))
		{
			found.push_back(person);
		}
	}

	return found;
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

		bool ftp_access = false;
		std::set<std::string>	ftp_persons;

		std::list<std::shared_ptr<Person>>	insert_list;
		std::list<std::shared_ptr<Person>>	update_list;

		int p_count = 0;
		if (true)
		{
			DbPersonQuery query(m_sql_conn);

			while (query.next())
			{
				++p_count;

				// check if person have samples

				if (query.solution_version != SOLUTION_VERSION)
				{
					if (!ftp_access)
					{
						auto person_list = ftp.get_list("");

						for (auto & p : person_list)
						{
#ifdef _DEBUG
							LOG_DEBUG("ftp samples for person " << p << "\n");
#endif
							ftp_persons.insert(p);
						}

						ftp_access = true;
					}

					if (ftp_persons.find(query.sample_url) == ftp_persons.cend())
					{
#ifdef _DEBUG
						LOG_DEBUG("person " << query.sample_url << " has not samples\n");
#endif
						continue;
					}

					// old features, generate new

					auto files = ftp.get_files(query.sample_url);
					if (files.empty())
						continue;

					auto person = std::make_shared<Person>(query.persone_id, query.sample_url, "");

					for (auto & fdata : files)
					{
						person->create_features(fdata);
					}

					if (person->features.empty())
						continue;

					if (query.solution_version < 0)
						insert_list.push_back(person);
					else
						update_list.push_back(person);

					person->version = query.solution_version;

					persons.push_back(person);
				}
				else
				{
					auto person = std::make_shared<Person>(query.persone_id, query.sample_url, query.key_features);
					if (person->features.empty())
						continue;

					LOG_DEBUG("person " << person->sample_url << " samples loaded\n");

					persons.push_back(person);
				}
			}
		}

		// store new results

		for (auto & person : insert_list)
		{
			LOG_DEBUG("person " << person->sample_url << " samples created\n");

			// insert recors

			DbPersonInsertSample q(m_sql_conn);
			q.execute(person->person_id, person->sample_url, person->get_features_json(), SOLUTION_VERSION);
		}

		for (auto & person : update_list)
		{
			LOG_DEBUG("person " << person->sample_url << " samples updated\n");

			// update record
			DbPersonUpdateSample q(m_sql_conn);
			q.execute(person->person_id, person->get_features_json(), SOLUTION_VERSION);
		}

		LOG_DEBUG("total " << p_count << " persons checked\n");
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

void PersonSet::store_id_to_sql_log(int camera_id, std::string const & person_id)
{
	DbPersonInsertLog q(m_sql_conn);

	q.execute(camera_id, person_id);
}
