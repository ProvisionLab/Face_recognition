#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sqlucode.h>

#include <string>
#include <memory>
//#include <stdexcept>
#include <vector>

namespace ODBC {

	template <int, int>
	class Handle;

	class Exception 
		: public std::exception
	{
	public:

		Exception(RETCODE rc, SQLHANDLE h, SQLSMALLINT ht);
	
		const char * what() const noexcept override;

	public:

		RETCODE		error_code;
		std::vector<std::string>	messages;
	};

	void throw_on_error(SQLRETURN rc, SQLHANDLE h, SQLSMALLINT ht);

	bool check_result(SQLRETURN rc, SQLHANDLE h, SQLSMALLINT ht);

	template <int htype, int hparenttype>
	class Handle
	{
	protected:

		static const int HandleType			= htype;
		static const int ParentHandleType	= hparenttype;

	public:

		Handle()
		{
			throw_on_error(SQLAllocHandle(htype, SQL_NULL_HANDLE, &m_Handle));
		}

		template <int hparenttype2>
		Handle( Handle<ParentHandleType, hparenttype2> const & parent )
		{
			throw_on_error(SQLAllocHandle(htype, parent.get(), &m_Handle));
		}

		~Handle()
		{
			SQLFreeHandle(htype, m_Handle);
		}

		SQLHANDLE get() const { return m_Handle; }

	protected:

		SQLRETURN throw_on_error(RETCODE rc)
		{
			ODBC::throw_on_error(rc, get(), HandleType);
			return rc;
		}

	private:

		// make noncopyable
		Handle(Handle const &);
		Handle & operator=(Handle const &);

	protected:

		SQLHANDLE	m_Handle;
	};


	class Environment 
		: public Handle<SQL_HANDLE_ENV, SQL_NULL_HANDLE>
	{
	public:

		Environment()
		{
			SetAttr(SQL_ATTR_ODBC_VERSION, SQL_OV_ODBC3);
		}

		~Environment()
		{
		}

		void SetAttr(SQLINTEGER attr, SQLUINTEGER value)
		{
			throw_on_error(SQLSetEnvAttr(m_Handle, attr, (SQLPOINTER)(uintptr_t)value, SQL_IS_UINTEGER));
		}
	};

	class Connection
		: public Handle<SQL_HANDLE_DBC, SQL_HANDLE_ENV>
	{
	public:

		Connection(std::shared_ptr<Environment> env = std::make_shared<Environment>())
			: Handle(*env)
			, env(env)
		{
		}

		~Connection()
		{
			disconnect();
		}

		std::shared_ptr<Environment> get_env()
		{
			return env;
		}

		bool is_open() const { return m_Handle != 0; }

		bool connect(Environment & env, std::string const & strConnect);
		bool connect(
			std::string const & host, std::string const & db_name, 
			std::string const & db_username, std::string const & db_password);

		void disconnect()
		{
			if (m_Handle != 0)
			{
				SQLDisconnect(m_Handle);
				SQLFreeHandle(HandleType, m_Handle);
				m_Handle = 0;
			}
		}

		// transactions

		void SetAutocommit(bool on) { SetAttr(SQL_ATTR_AUTOCOMMIT, on ? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF); }

		void Commit()
		{
			throw_on_error(SQLEndTran(HandleType, get(), SQL_COMMIT));
		}

		void Rollback()
		{
			throw_on_error(SQLEndTran(HandleType, get(), SQL_ROLLBACK));
		}

		void SetAttr(SQLINTEGER attr, SQLUINTEGER value)
		{
			throw_on_error(SQLSetConnectAttr(get(), attr, (SQLPOINTER)(uintptr_t)value, SQL_IS_UINTEGER));
		}

	private:

		std::shared_ptr<Environment> env;
	};

	class Stmt
		: public Handle<SQL_HANDLE_STMT, SQL_HANDLE_DBC>
	{
	public:

		Stmt(Connection & conn)
			: Handle(conn)
		{
		}

		~Stmt()
		{
			SQLFreeStmt(m_Handle, SQL_CLOSE);
		}

		void Prepare(LPCSTR pszSql)
		{
			throw_on_error(SQLPrepareA(get(), (SQLCHAR*)pszSql, SQL_NTS));
		}

		SQLRETURN Execute()
		{
			SQLRETURN rc = throw_on_error(SQLExecute(get()));
			return rc;
		}

		bool Fetch()
		{
			SQLRETURN rc = SQLFetch(get());
			if (rc == SQL_NO_DATA)
				return false;
			throw_on_error(rc);
			return true;// SQL_SUCCEEDED(rc);// != SQL_NO_DATA;
		}

		void Bind(int index, int c_type, void * pData, int nSize, SQLLEN * outLen)
		{
			throw_on_error(SQLBindCol(get(), index, c_type, pData, nSize, outLen));
		}

		void Bind(int index, int & value)
		{
			return Bind(index, SQL_C_LONG, &value, 0, NULL);
		}

		void Bind(int index, std::string & value)
		{

		}

		void get_data(int index, std::string & output)
		{
			SQLLEN len;
			throw_on_error(SQLGetData(get(), index, SQL_C_CHAR, (void*)-1, 0, &len));

			output.clear();

			if (len == SQL_NULL_DATA)
				return;

			while (true)
			{
				size_t nLen = output.size();
				if (len == SQL_NO_TOTAL)
					len = (nLen + nLen / 2 + 16) * sizeof(char);

				output.resize(len / sizeof(char) + 1);
				SQLLEN buflen = (output.size() - nLen) * sizeof(char);

				throw_on_error(SQLGetData(get(), index, SQL_C_CHAR, (char*)output.data() + nLen, buflen, &len));

				if (len > 0)
				{
					if (len <= buflen)
					{
						output.resize(len / sizeof(char) + nLen);
						break;
					}

					len += nLen * sizeof(char);
				}

				if (output[buflen-1] == '\0')
					--buflen;

				nLen += buflen / sizeof(char);
				output.resize(nLen);
			}
		}
	}; // class Stmt

} // namespace ODBC
