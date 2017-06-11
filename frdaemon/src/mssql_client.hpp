#pragma once

#include <string>
#include <vector>

class SqlConnection
{
public:

	SqlConnection();

	bool connect(std::string const & host, std::string const & db_name, std::string const & db_username, std::string const & db_password);

};


class PersoneQuery
{
public:

	PersoneQuery(SqlConnection &conn);

	bool next();

public:

	int	persone_id;
	std::string	persone_resource_id;

	std::vector<float>	key_features;
	int	solution_version;
};
