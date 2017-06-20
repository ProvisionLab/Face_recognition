#include "mssql_client.hpp"


DbPersonQuery::DbPersonQuery(ODBC::Connection & conn)
	: ODBC::Stmt(conn)
{
	Prepare("SELECT Id, SolutionVersion, KeyFeatures, CONVERT(nchar(36),Id) FROM casino.FaceRecognitionResources");

	Execute();
}

bool DbPersonQuery::next()
{
	if (!Fetch())
		return false;

	get_uuid_column(1, person_id);

	if (!get_column(2, solution_version))
		solution_version = -1;

	get_column(3, key_features);

	get_column(4, person_desc);

	return true;
}

DbPersonQuerySamples::DbPersonQuerySamples(ODBC::Connection &conn, ODBC::UUID person_id)
	: ODBC::Stmt(conn)
{
	Prepare("SELECT SampleUrl FROM casino.FaceRecognitionSamples WHERE FaceRecognitionResourceId=?");

	BindUuidI(1, person_id);

	Execute();
}

bool DbPersonQuerySamples::next()
{
	if (!Fetch())
		return false;

	get_column(1, sample_url);

	return true;
}

DbPersonUpdateSample::DbPersonUpdateSample(ODBC::Connection &conn)
	: ODBC::Stmt(conn)
{
	Prepare("UPDATE casino.FaceRecognitionResources SET KeyFeatures=?, SolutionVersion=? WHERE Id=?");
}

void DbPersonUpdateSample::execute(ODBC::UUID person_id, std::string const & key_features, long solution_version)
{
	BindI(1, key_features);
	BindI(2, solution_version);
	BindUuidI(3, person_id);

	Execute();
}

DbPersonInsertLog::DbPersonInsertLog(ODBC::Connection &conn)
	: ODBC::Stmt(conn)
{
	Prepare("INSERT INTO casino.FaceRecognitionLogs (FaceCameraId, PersonId) VALUES (?,?)");
}

void DbPersonInsertLog::execute(long camera_id, ODBC::UUID person_id)
{
	BindI(1, camera_id);
	BindUuidI(2, person_id);

	Execute();
}
