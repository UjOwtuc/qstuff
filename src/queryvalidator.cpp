#include "queryvalidator.h"

#ifndef WITH_LOGSTUFF_QUERY

qint32 check_parseable(QueryValidator::ParseRule input_type, const char* query_string)
{
	QString value(query_string);
	qint32 error = 0;
	switch (input_type)
	{
		case QueryValidator::Query:
			error = -1;
			break;
		case QueryValidator::Field:
			if (value.length() > 0)
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
			break;
		case QueryValidator::Scalar:
		{
			bool ok;
			value.toDouble(&ok);
			if (ok)
				error = -1;
			else if (value.length() > 1 && value.front() == '"' && value.back() == '"')
				error = -1;
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
}


QValidator::State QueryValidator::validate(QString& input, int&) const
{
	auto error = check_parseable(m_rule, input.toUtf8().constData());
	QValidator::State state = Intermediate;
	if (error == -1)
		state = Acceptable;
	return state;
}


void QueryValidator::fixup(QString& input) const
{
	if (input.length() > 0 && check_parseable(m_rule, input.toUtf8().constData()) != -1)
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
