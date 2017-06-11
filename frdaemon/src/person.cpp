#include "person.hpp"
#include "ftp_client.hpp"
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include "frame_features.hpp"
#include "mssql_client.hpp"

#if !defined(USE_DAEMON) && defined(_DEBUG)

#include <iostream>
#define LOG_DEBUG(x)	std::cerr << x

#else

#define LOG_DEBUG(x)	(0)

#endif

extern std::atomic<bool> sig_term;

Person::Person(PersoneQuery & query)
{
	guid = query.persone_resource_id;
}

Person::Person(std::ifstream & f)
{
	std::uint16_t sz;
	
	f.read((char*)&sz, sizeof(sz));
	if (f.fail()) return;

	guid.resize(sz);

	f.read((char*)guid.data(), guid.size());
	if (f.fail())
	{
		guid.clear();
		return;
	}

	files["0"] = cv::Mat();

	// load from file
	// 2do:
}

void Person::save(std::ofstream & f)
{
	std::uint16_t sz = std::uint16_t(guid.size());
	f.write((char*)&sz, sizeof(sz));
	f.write(guid.data(), guid.size());

	// save to file
	// 2do:
}

size_t Person::get_memory_usage()
{
	// 2do:

	size_t sz = guid.size();
	for (auto & f : files)
	{
		sz += f.first.size();
		auto ms = f.second.size();
		sz += ms.width * ms.height * 4;
	}

	return sz;
}

void Person::append_sample(std::string const & fn, std::vector<uint8_t> data)
{
	try
	{
		cv::Mat image = cv::imdecode(cv::InputArray(data), -1);

		if (image.data)
		{
			files[fn] = std::move(image);
		}

	}
	catch (...)
	{
	}
}

PersonSet::PersonSet()
{

}

PersonSet::~PersonSet()
{
	if (!swap_file_name.empty())
	{
		fs::remove(fs::path(swap_file_name));
	}
}

void PersonSet::load_from_ftp(std::string const & ftp_url)
{
	FtpClient ftp;
	ftp.ftp_url = ftp_url;

	auto persone_list = ftp.get_list("");

	std::ofstream swp;

	size_t memory_usage = 0;

	if (swap_mode)
	{
		fs::path f = fs::temp_directory_path();

		f /= "frd-swap.tmp";

		swap_file_name = f.string();

		swp.open(swap_file_name, std::ios::binary);
	}
	
	for (auto & guid : persone_list)
	{
		LOG_DEBUG("person guid: " << guid << std::endl);

		Person person(guid);

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
				person.append_sample(fn, std::move(fdata));
			}
			else
			{
				LOG_DEBUG(" fail" << std::endl);
			}
		}

		if (!person.files.empty())
		{
			if (swap_mode && memory_usage >= max_memory_usage)
			{
				person.save(swp);
			}
			else
			{
				memory_usage += person.get_memory_usage();

				persons.push_back(std::make_shared<Person>(std::move(person)));
			}
		}
	}
}

std::vector<std::shared_ptr<Person>> PersonSet::recognize(cv::Mat const & frame)
{
	std::vector<std::shared_ptr<Person>> found;

	FrameFeatures ff;
	ff.generate(frame);

	for (auto & person : persons)
	{
		if (sig_term)
			return{};

		if (ff.contains_person(*person))
		{
			found.push_back(person);
		}
	}

	if (swap_mode)
	{
		std::atomic<bool> eof(false);

		std::mutex  mutex;
		std::condition_variable cv;
		std::list<std::shared_ptr<Person>> shared_persons;

		std::thread swap_loader([&]() 
		{
			std::ifstream swp(swap_file_name, std::ios::binary);
			swp.seekg(0);

			std::list<std::shared_ptr<Person>> ps;

			while (swp.good() && !sig_term)
			{
				Person person(swp);

				if (person.guid.empty())
					break;

				ps.push_back(std::make_shared<Person>(std::move(person)));

				if (ps.size() >= preload_persons_threshold)
				{
					std::unique_lock<std::mutex> l(mutex);

					// prevent OOM if reading from disk is faster then recognition
					while (shared_persons.size() > preload_persons_threshold)
					{
						l.unlock();
						std::this_thread::yield();
						l.lock();
					}

					shared_persons.splice(shared_persons.end(), ps);

					cv.notify_all();
				}
			}

			if (!ps.empty())
			{
				std::lock_guard<std::mutex> l(mutex);
				shared_persons.splice(shared_persons.end(), ps);
				cv.notify_all();
			}

			eof = true;
		});

		std::list<std::shared_ptr<Person>> ps;

		while (!eof)
		{
			{
				// take next portion of data
				std::unique_lock<std::mutex> l(mutex);

				while (!eof && shared_persons.empty())
				{
					cv.wait_for(l, std::chrono::milliseconds(100));
				}

				ps.splice(ps.end(), shared_persons);
			}

			// recognize that portion

			while (!ps.empty())
			{
				if (ff.contains_person(*ps.front()))
				{
					found.push_back(ps.front());
				}

				ps.pop_front();
			}
		}

		swap_loader.join();
	}

	return found;
}

bool PersonSet::load_from_sql(std::string const & host, std::string const & db_name, std::string const & db_username, std::string const & db_password)
{
	ODBC::Connection conn;

	if (!conn.connect(host, db_name, db_username, db_password))
		return false;

	PersoneQuery query(conn);

	while (query.next())
	{
		persons.push_back(std::make_shared<Person>(query));
	}

	return persons.size() > 0;
}
