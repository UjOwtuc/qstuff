#ifndef QUERYVALIDATOR_H
#define QUERYVALIDATOR_H

#include <QValidator>

class QueryValidator : public QValidator
{
	Q_OBJECT
	Q_PROPERTY(ParseRule rule MEMBER m_rule WRITE setRule USER true);
	Q_PROPERTY(bool acceptEmpty MEMBER m_acceptEmpty WRITE setAcceptEmpty);

public:
	enum ParseRule {
		Query = 0,
		Field = 1,
		Scalar = 2,
		List = 3,
	};

	explicit QueryValidator(ParseRule rule, QObject* parent = nullptr);
	QValidator::State validate(QString& input, int& /*pos*/) const override;
	void fixup(QString& input) const override;

	void setRule(ParseRule rule) { m_rule = rule; }
	void setAcceptEmpty(bool accept) { m_acceptEmpty = accept; }

private:
	ParseRule m_rule;
	bool m_acceptEmpty;
};
Q_DECLARE_METATYPE(QueryValidator::ParseRule);

qint32 check_parseable(QueryValidator::ParseRule rule, const QString& value);

#endif // QUERYVALIDATOR_H
