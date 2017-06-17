#include "mssql_client.hpp"


DbPersonQuery::DbPersonQuery(ODBC::Connection & conn)
	: ODBC::Stmt(conn)
{
	//CONVERT(nchar(36), A.Id)
#if USE_SAMPLES_IN_TWO_TABLES
	Prepare("SELECT A.Id, C.SolutionVersion, C.KeyFeatures, CONVERT(nchar(36),A.Id) FROM casino.Persons AS A "
		"LEFT JOIN casino.FaceRecognitionSamples AS B ON A.Id = B.Id "
		"LEFT JOIN casino.FaceRecognitionResources AS C ON B.FaceRecognitionResourceId = C.Id");
#else
	Prepare("SELECT CONVERT(nchar(36),A.Id), SolutionVersion, KeyFeatures FROM casino.Persons AS A LEFT JOIN casino.FaceRecognitionSamples AS B ON A.Id = B.Id");
#endif

	Execute();
}

bool DbPersonQuery::next()
{
	if (!Fetch())
		return false;

	get_uuid_column(1, persone_id);

	if (!get_column(2, solution_version))
		solution_version = -1;

	get_column(3, key_features);

	get_column(4, sample_url);

	return true;
}

DbPersonInsertSample::DbPersonInsertSample(ODBC::Connection &conn)
	: ODBC::Stmt(conn)
{
#if USE_SAMPLES_IN_TWO_TABLES
	Prepare("INSERT INTO casino.FaceRecognitionResources (KeyFeatures,SolutionVersion)"
		" OUTPUT ?, INSERTED.Id, ? INTO casino.FaceRecognitionSamples(Id, FaceRecognitionResourceId, SampleUrl)"
		" VALUES(?,?)");
#else
	Prepare("INSERT INTO casino.FaceRecognitionSamples (Id, KeyFeatures, SolutionVersion) VALUES (?,?,?)");
#endif
}

void DbPersonInsertSample::execute(ODBC::UUID person_id, std::string const & sample_url, std::string const & key_features, long solution_version)
{
	BindUuidI(1, person_id);
	BindI(2, sample_url);
	BindI(3, key_features);
	BindI(4, solution_version);

	Execute();
}

DbPersonUpdateSample::DbPersonUpdateSample(ODBC::Connection &conn)
	: ODBC::Stmt(conn)
{
#if USE_SAMPLES_IN_TWO_TABLES
	Prepare("UPDATE C SET C.KeyFeatures=?, C.SolutionVersion=?"
		" FROM casino.FaceRecognitionSamples AS B INNER JOIN casino.FaceRecognitionResources AS C ON FaceRecognitionResourceId = C.Id"
		" WHERE B.Id=?");
#else
	Prepare("UPDATE casino.FaceRecognitionSamples SET KeyFeatures=?, SolutionVersion=? WHERE Id=?");
#endif
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

void DbPersonInsertLog::execute(long camera_id, std::string const & person_id)
{
	BindI(1, camera_id);
	BindI(2, person_id);

	Execute();
}
