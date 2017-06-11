#include "mssql_client.hpp"


SqlConnection::SqlConnection()
{

}

bool SqlConnection::connect(std::string const & host, std::string const & db_name, std::string const & db_username, std::string const & db_password)
{
	// 2do:
	return false;
}



PersoneQuery::PersoneQuery(SqlConnection & conn)
{
	// 2do:
}

bool PersoneQuery::next()
{
	// 2do:
	return false;
}
