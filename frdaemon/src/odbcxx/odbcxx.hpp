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

	template <typename _Tx>
	class BindValue
	{
	public:
	private:

		_Tx * value = nullptr;
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

		void BindI(int index, long const & input)
		{
			throw_on_error(SQLBindParameter(get(), index, SQL_PARAM_INPUT,
				SQL_C_LONG, SQL_INTEGER, 0, 0, const_cast<long*>(&input), 0, NULL));
		}

		void BindI(int index, std::string const & input)
		{
			size_t buflen = input.size() + 1;

			if (!input.empty())
			{
				throw_on_error(SQLBindParameter(get(), index, SQL_PARAM_INPUT,
					SQL_C_CHAR, SQL_VARCHAR, input.size(), 0, const_cast<char*>(input.c_str()), buflen, NULL));
			}
			else
			{
				throw_on_error(SQLBindParameter(get(), index, SQL_PARAM_INPUT,
					SQL_C_CHAR, SQL_VARCHAR, 1, 0, NULL, 0, NULL));
			}
		}

		void Bind(int index, int c_type, void * pData, int nSize, SQLLEN * outLen)
		{
			throw_on_error(SQLBindCol(get(), index, c_type, pData, nSize, outLen));
		}

		void Bind(int index, int & value)
		{
			return Bind(index, SQL_C_LONG, &value, 0, NULL);
		}

		void Bind(int index, std::string & value, size_t max_size)
		{

		}

		bool get_data(int index, long & output)
		{
			SQLLEN len;
			throw_on_error(SQLGetData(get(), index, SQL_C_LONG, &output, sizeof(output), &len));
			return len != SQL_NULL_DATA;
		}

		bool get_data(int index, std::string & output)
		{
			SQLLEN len;

			char buf[16];
			throw_on_error(SQLGetData(get(), index, SQL_C_CHAR, (char*)buf, sizeof(buf), &len));

			output.clear();

			if (len == SQL_NULL_DATA)
			{
				output.clear();
				return false;
			}

			if (len != SQL_NO_TOTAL)
			{
				output.assign(buf, len-1);
				return true;
			}

			output.assign(buf, sizeof(buf)*sizeof(char)-1);

			while (true)
			{
				size_t start = output.size();

				output.resize(start + start / 2 + 16);
				SQLLEN bufSize = (output.size() - start) * sizeof(char);

				throw_on_error(SQLGetData(get(), index, SQL_C_CHAR, (char*)output.data() + start, bufSize, &len));

				if (len > 0)
				{
					output.resize(start + len);
					break;
				}

				output.pop_back();
			}

			return true;
		}
	}; // class Stmt

} // namespace ODBC
