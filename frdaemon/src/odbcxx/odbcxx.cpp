#include "odbcxx.hpp"

#include <array>

namespace ODBC {

	std::vector<std::string> get_error_messages(RETCODE rc, SQLHANDLE h, SQLSMALLINT ht)
	{
		std::vector<std::string> v;

		if (rc == SQL_ERROR || rc == SQL_SUCCESS_WITH_INFO)
		{
			SQLSMALLINT iRec = 0;
			SQLINTEGER  iError;
			std::array<SQLCHAR, 1000> szMessage;
			SQLCHAR	szState[SQL_SQLSTATE_SIZE + 1];

			while (SQLGetDiagRecA(ht, h,
				++iRec,
				szState,
				&iError,
				szMessage.data(),
				(SQLSMALLINT)(szMessage.size()),
				(SQLSMALLINT *)NULL) == SQL_SUCCESS)
			{
				char szout[1000];
				sprintf(szout, "[%5.5s] %s (%d)\n", szState, szMessage.data(), iError);

				v.push_back(szout);
			}
		}
		else
		{
			switch (rc)
			{
			case SQL_INVALID_HANDLE:
				v.push_back("SQL_INVALID_HANDLE");
				break;
			}
		}

		return v;
	}

	Exception::Exception(RETCODE rc, SQLHANDLE h, SQLSMALLINT ht)
		: error_code(rc)
		, messages(get_error_messages(rc, h, ht))
	{
		if (messages.empty())
			messages.push_back(std::to_string(rc));
	}

	const char * Exception::what() const noexcept
	{
		return messages[0].c_str();
	}

	void throw_on_error(SQLRETURN rc, SQLHANDLE h, SQLSMALLINT ht)
	{
		if (!SQL_SUCCEEDED(rc))
			throw Exception(rc, h, ht);
	}

	bool check_result(SQLRETURN rc, SQLHANDLE h, SQLSMALLINT ht)
	{
		bool result = SQL_SUCCEEDED(rc);

		if (rc != SQL_SUCCESS)
		{
			switch (rc)
			{
			case SQL_ERROR:
			case SQL_SUCCESS_WITH_INFO:
				break;

			case SQL_INVALID_HANDLE:
				//LOG(LOG_ERR, "SQL_INVALID_HANDLE");
				return result;

			case SQL_NEED_DATA:
			case SQL_NO_DATA:
			default:
				return result;
			}

#ifdef _WIN32
#ifdef _DEBUG
			auto v = get_error_messages(rc, h, ht);

			for (auto & s : v)
			{
				OutputDebugStringA(s.c_str());
			}
#endif
#endif

		}

		return result;
	}

	bool Connection::connect(
		std::string const & db_host, 
		std::string const & db_name, 
		std::string const & db_username, 
		std::string const & db_password)
	{
		// 2do:
#ifdef _WIN32
		std::string strConnect("DRIVER={SQL Server Native Client 11.0};Trusted_Connection=yes;");

		strConnect += "SERVER=" + db_host + ";";
		strConnect += "Database=" + db_name + ";";
#else
		std::string strConnect("DRIVER={FreeTDS};");// TDS_Version = 8.0; ");

		strConnect += "SERVER=" + db_host + ";";
		strConnect += "PORT=1433;";
//		strConnect += "DataSource=" + db_host + ";";
		strConnect += "Database=" + db_name + ";";
//		strConnect += "InitialCatalog=" + db_name + ";";
#endif


		if (!db_username.empty())
			strConnect += "UID=" + db_username + ";";

		if (!db_password.empty())
			strConnect += "PWD=" + db_password + ";";

		RETCODE rc;

		// Set login timeout to 10 seconds
		SetAttr(SQL_LOGIN_TIMEOUT, 10);

		rc = SQLDriverConnectA(m_Handle,
			NULL, // we're not interested in spawning a window
			(SQLCHAR*)strConnect.data(), (SQLSMALLINT)strConnect.size(),
			NULL, 0, NULL,
			SQL_DRIVER_NOPROMPT);

		throw_on_error(rc);

		return check_result(rc, get(), HandleType);
	}

} // namespace ODBC
