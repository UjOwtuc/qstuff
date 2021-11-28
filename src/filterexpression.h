#ifndef FILTEREXPRESSION_H
#define FILTEREXPRESSION_H

#include <QMetaType>
#include <QMap>

class QSettings;

class FilterExpression
{
public:
	enum Op {Eq=0, Lt, Le, Gt, Ge, In, Like};

	static const QMap<Op, QString>& ops();
	static QString opString(Op op);
	static Op invertOp(Op op);

	FilterExpression();
	FilterExpression(const QString& id, Op op, const QString& value, bool inverted);
	FilterExpression(const FilterExpression& other);

	FilterExpression& operator=(const FilterExpression& rhs);
	bool operator<(const FilterExpression& rhs) const;
	bool operator==(const FilterExpression& rhs) const;
	bool operator!=(const FilterExpression& rhs) const;

	QString toString() const;
	FilterExpression inverted() const;
	bool enabled() const { return m_enabled; }
	void setEnabled(bool enabled) { m_enabled = enabled; }
	void setLabel(const QString& label) { m_label = label; }

	const QString& id() const { return m_id; }
	Op op() const { return m_op; }
	const QString& value() const { return m_value; }
	QString label() const;
	bool hasCustomLabel() const { return !m_label.isEmpty(); }
	bool isInverted() const { return m_inverted; }
	bool isValid() const { return m_isValid; }

	bool isEmpty() const;

private:
	QString m_id;
	Op m_op;
	QString m_value;
	bool m_enabled;
	QString m_label;
	bool m_inverted;
	bool m_isValid;
};
Q_DECLARE_METATYPE(FilterExpression);


void saveFiltersArray(QSettings& settings, const QList<FilterExpression>& filters);
QList<FilterExpression> loadFiltersArray(QSettings& settings);

#endif // FILTEREXPRESSION_H
