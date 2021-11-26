#include "queryvalidator.h"

#include <QDebug>

#ifdef WITH_LOGSTUFF_QUERY

extern "C" {
void* init_parsers();
void delete_parsers(void* parsers);  // unused for now, our single parsers instance is never freed
qint32 test_parse_query(void* parsers, const char* query_string);
qint32 test_parse_identifier(void* parsers, const char* query_string);
qint32 test_parse_scalar(void* parsers, const char* query_string);
qint32 test_parse_list(void* parsers, const char* query_string);
}

namespace {
	void* parsers = nullptr;
}

qint32 check_parseable(QueryValidator::ParseRule rule, const QString& value)
{
	if (! parsers)
	{
		qDebug() << "initializing parsers";
		parsers = init_parsers();
	}
	QByteArray raw = value.toUtf8();
	const char* s = raw.constData();
	switch (rule)
	{
		case QueryValidator::Query: return test_parse_query(parsers, s);
		case QueryValidator::Field: return test_parse_identifier(parsers, s);
		case QueryValidator::Scalar: return test_parse_scalar(parsers, s);
		case QueryValidator::List: return test_parse_list(parsers, s);
	}
	qFatal("unhandled rule type: %d", rule);
}

#else // WITH_LOGSTUFF_QUERY

qint32 check_parseable(QueryValidator::ParseRule input_type, const QString& value)
{
	qint32 error = 0;
	switch (input_type)
	{
		case QueryValidator::Query:
			error = -1;
			break;
		case QueryValidator::Field:
			if (value.length() > 0)
			{
				static QString validFirstChar = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
				if (!validFirstChar.contains(value[0]))
					error = 0;
				else
				{
					static QString validChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-_.";
					error = -1;
					for (int pos=0; pos<value.length(); ++pos)
					{
						if (!validChars.contains(value[pos]))
						{
							error = pos;
							break;
						}
					}
				}
			}
			break;
		case QueryValidator::Scalar:
		{
			bool ok;
			value.toDouble(&ok);
			if (ok)
				error = -1;
			else if (value.length() > 1 && value.front() == '"' && value.back() == '"')
				error = -1;

			if (value.startsWith("0") && !value.startsWith("0."))
				error = 0;
			break;
		}
		case QueryValidator::List:
			if (value.length() > 1 && value.front() == '(' && value.back() == ')')
				error = -1;
	}
	return error;
}

#endif

QueryValidator::QueryValidator(ParseRule rule, QObject* parent)
	: QValidator(parent)
{
	m_rule = rule;
	m_acceptEmpty = false;
}


QValidator::State QueryValidator::validate(QString& input, int&) const
{
	if (m_acceptEmpty && input.isEmpty())
		return Acceptable;

	auto error = check_parseable(m_rule, input);
	QValidator::State state = Intermediate;
	if (error == -1)
		state = Acceptable;
	return state;
}


void QueryValidator::fixup(QString& input) const
{
	if (input.length() > 0 && check_parseable(m_rule, input) != -1)
	{
		switch (m_rule)
		{
			case Scalar:
				if (input.front() != '"')
					input.prepend('"');
				if (input.back() != '"')
					input.append('"');
				break;
			case List:
				if (input.back() != ')')
					input.append(')');
				if (input.front() != '(')
					input.prepend('(');
			default:
				break;
		}
	}
}
