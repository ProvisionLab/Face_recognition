#include "mssql_client.hpp"

PersoneQuery::PersoneQuery(ODBC::Connection & conn)
	: conn(conn)
	, stmt(conn)
{
	stmt.Prepare("SELECT id, FaceRecognitionResourceId, SampleUrl FROM FaceRecognitionSamples");

	SQLRETURN rc = stmt.Execute();

	if (!SQL_SUCCEEDED(rc))
		throw std::runtime_error(__FUNCTION__);

	stmt.Bind(1, persone_id);
	stmt.Bind(2, persone_resource_id);
}

bool PersoneQuery::next()
{
	if (!stmt.Fetch())
		return false;

	stmt.get_data(3, persone_sample_url);


	//ODBC::Stmt stmt_res(conn);

	return true;
}
