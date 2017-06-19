#pragma once

#include "odbcxx/odbcxx.hpp"
#include <vector>

#define USE_SAMPLES_IN_TWO_TABLES 1

class DbPersonQuery
	: protected ODBC::Stmt
{
public:

	DbPersonQuery(ODBC::Connection &conn);

	bool next();

public:

	ODBC::UUID		person_id;
	std::string		person_desc;
	std::string		key_features;
	long solution_version = 0;

private:

};

class DbPersonQuerySamples
	: protected ODBC::Stmt
{
public:

	DbPersonQuerySamples(ODBC::Connection &conn, ODBC::UUID person_id);

	bool next();

public:

	std::string		sample_url;
};

class DbPersonUpdateSample
	: protected ODBC::Stmt
{
public:

	DbPersonUpdateSample(ODBC::Connection &conn);

	void execute(ODBC::UUID person_id, std::string const & key_features, long solution_version);
};

class DbPersonInsertLog
	: protected ODBC::Stmt
{
public:

	DbPersonInsertLog(ODBC::Connection &conn);

	void execute(long camera_id, ODBC::UUID person_id);
};
