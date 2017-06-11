#pragma once

#include "odbcxx/odbcxx.hpp"
#include <vector>

class PersoneQuery
{
public:

	PersoneQuery(ODBC::Connection &conn);

	bool next();

public:

	int	persone_id = 0;
	int	persone_resource_id = 0;
	std::string		persone_sample_url;

	std::vector<float>	key_features;
	int	solution_version;

private:

	ODBC::Connection & conn;
	ODBC::Stmt stmt;
};
