#pragma once

#include "odbcxx/odbcxx.hpp"
#include <vector>

class DbPersonQuery
	: protected ODBC::Stmt
{
public:

	DbPersonQuery(ODBC::Connection &conn);

	bool next();

public:

	std::string		persone_id;
	std::string		key_features;
	long solution_version = 0;

private:

};

class DbPersonInsertSample 
	: protected ODBC::Stmt
{
public:

	DbPersonInsertSample(ODBC::Connection &conn);

	void execute(std::string const & id, std::string const & key_features, long solution_version);
};

class DbPersonUpdateSample
	: protected ODBC::Stmt
{
public:

	DbPersonUpdateSample(ODBC::Connection &conn);

	void execute(std::string const & id, std::string const & key_features, long solution_version);
};
