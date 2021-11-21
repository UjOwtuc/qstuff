#ifndef QUERYVALIDATOR_H
#define QUERYVALIDATOR_H

#include <QValidator>

class QueryValidator : public QValidator
{
	Q_PROPERTY(ParseRule rule READ rule WRITE setRule USER true);
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

	ParseRule rule() const { return m_rule; }
	void setRule(ParseRule value) { m_rule = value; }

private:
	ParseRule m_rule;
};

qint32 check_parseable(QueryValidator::ParseRule rule, const QString& value);

#endif // QUERYVALIDATOR_H
