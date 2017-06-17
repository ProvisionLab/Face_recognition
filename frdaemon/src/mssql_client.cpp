#include "mssql_client.hpp"

DbPersonQuery::DbPersonQuery(ODBC::Connection & conn)
	: ODBC::Stmt(conn)
{
	Prepare("SELECT A.Id, SolutionVersion, KeyFeatures FROM casino.Persons AS A LEFT JOIN casino.FaceRecognitionSamples AS B ON A.Id = B.Id");

	Execute();
}

bool DbPersonQuery::next()
{
	if (!Fetch())
		return false;

	get_data(1, persone_id);

	if (!get_data(2, solution_version))
		solution_version = -1;

	get_data(3, key_features);

	return true;
}

DbPersonInsertSample::DbPersonInsertSample(ODBC::Connection &conn)
	: ODBC::Stmt(conn)
{
	Prepare("INSERT INTO casino.FaceRecognitionSamples (Id, KeyFeatures, SolutionVersion) VALUES (?,?,?)");
}

void DbPersonInsertSample::execute(std::string const & id, std::string const & key_features, long solution_version)
{
	BindI(1, id);
	BindI(2, key_features);
	BindI(3, solution_version);

	Execute();
}

DbPersonUpdateSample::DbPersonUpdateSample(ODBC::Connection &conn)
	: ODBC::Stmt(conn)
{
	Prepare("UPDATE casino.FaceRecognitionSamples SET KeyFeatures=?, SolutionVersion=? WHERE Id=?");
}

void DbPersonUpdateSample::execute(std::string const & id, std::string const & key_features, long solution_version)
{
	BindI(1, key_features);
	BindI(2, solution_version);
	BindI(3, id);

	Execute();
}

DbPersonInsertLog::DbPersonInsertLog(ODBC::Connection &conn)
	: ODBC::Stmt(conn)
{
	Prepare("INSERT INTO casino.FaceRecognitionLogs (FaceCameraId, PersonId) VALUES (?,?)");
}

void DbPersonInsertLog::execute(long camera_id, std::string const & person_id)
{
	BindI(1, camera_id);
	BindI(2, person_id);

	Execute();
}
