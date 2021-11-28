#include "filterexpression.h"
#include "queryvalidator.h"

#include <QSettings>


void saveFiltersArray(QSettings& settings, const QList<FilterExpression>& filters)
{
	int size = filters.size();
	settings.beginWriteArray("filters", size);
	for (int i=0; i<size; ++i)
	{
		settings.setArrayIndex(i);
		const FilterExpression& filter = filters.at(i);
		settings.setValue("id", filter.id());
		settings.setValue("op", filter.op());
		settings.setValue("value", filter.value());
		if (filter.hasCustomLabel())
			settings.setValue("label", filter.label());
		else
			settings.setValue("label", QString());
		settings.setValue("inverted", filter.isInverted());
		settings.setValue("enabled", filter.enabled());
	}
	settings.endArray();
}


QList<FilterExpression> loadFiltersArray(QSettings& settings)
{
	QList<FilterExpression> filters;
	int size = settings.beginReadArray("filters");
	for (int i=0; i<size; ++i)
	{
		settings.setArrayIndex(i);
		QString id = settings.value("id").toString();
		FilterExpression::Op op = static_cast<FilterExpression::Op>(settings.value("op").toInt());
		QString value = settings.value("value").toString();
		QString label = settings.value("label").toString();
		bool inverted = settings.value("inverted").toBool();
		bool enabled = settings.value("enabled").toBool();

		FilterExpression expr(id, op, value, inverted);
		expr.setLabel(label);
		expr.setEnabled(enabled);
		filters << expr;
	}
	settings.endArray();
	return filters;
}


namespace {
	QMap<FilterExpression::Op, QString>* opStrings = nullptr;

	void fillGlobalMaps()
	{
		if (! opStrings)
		{
			opStrings = new QMap<FilterExpression::Op, QString>();
			opStrings->insert(FilterExpression::Eq, "=");
			opStrings->insert(FilterExpression::Lt, "<");
			opStrings->insert(FilterExpression::Le, "<=");
			opStrings->insert(FilterExpression::Gt, ">");
			opStrings->insert(FilterExpression::Ge, ">=");
			opStrings->insert(FilterExpression::In, "in");
			opStrings->insert(FilterExpression::Like, "like");
		}
	}
}


const QMap<FilterExpression::Op, QString>& FilterExpression::ops()
{
	fillGlobalMaps();
	return *opStrings;
}


QString FilterExpression::opString(FilterExpression::Op op)
{
	fillGlobalMaps();
	return opStrings->value(op);
}


FilterExpression::FilterExpression()
{
	m_op = Eq;
	m_enabled = true;
	m_inverted = false;
	m_isValid = false;
}


FilterExpression::FilterExpression(const FilterExpression& other)
{
	*this = other;
}


FilterExpression::FilterExpression(const QString& id, FilterExpression::Op op, const QString& value, bool inverted)
	: m_id(id), m_op(op), m_value(value), m_enabled(true), m_inverted(inverted)
{
	m_isValid = check_parseable(QueryValidator::Query, toString()) == -1;
}


FilterExpression& FilterExpression::operator=(const FilterExpression& rhs)
{
	m_id = rhs.m_id;
	m_op = rhs.m_op;
	m_value = rhs.m_value;
	m_enabled = rhs.m_enabled;
	m_label = rhs.m_label;
	m_inverted = rhs.m_inverted;
	m_isValid = rhs.m_isValid;
	return *this;
}


bool FilterExpression::operator<(const FilterExpression& rhs) const
{
	return toString() < rhs.toString();
}


bool FilterExpression::operator==(const FilterExpression& rhs) const
{
	return (isEmpty() && rhs.isEmpty()) || toString() == rhs.toString();
}


bool FilterExpression::operator!=(const FilterExpression& rhs) const
{
	return toString() != rhs.toString();
}


QString FilterExpression::toString() const
{
	return QString("%1%2 %3 %4").arg(m_inverted ? "not " : "", m_id, opString(m_op), m_value);
}

FilterExpression FilterExpression::inverted() const
{
	FilterExpression inv(*this);
	inv.m_inverted = !m_inverted;

	if (inv.hasCustomLabel())
	{
		if (inv.m_label.startsWith("not "))
			inv.m_label = inv.m_label.mid(4);
		else
			inv.m_label = "not " + inv.m_label;
	}
	return inv;
}


QString FilterExpression::label() const
{
	if (m_label.isEmpty())
		return toString();
	return m_label;
}


bool FilterExpression::isEmpty() const
{
	return m_id.isEmpty() && m_value.isEmpty();
}
