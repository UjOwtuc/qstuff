#include "filtermodel.h"
#include "queryvalidator.h"

#include <QBrush>
#include <QDebug>
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


FilterModel::FilterModel(QObject* parent)
	: QAbstractListModel(parent)
{}


int FilterModel::rowCount(const QModelIndex& /*parent*/) const
{
	return m_filters.size();
}


QVariant FilterModel::data(const QModelIndex& index, int role) const
{
	QVariant data;
	if (index.row() >= 0 && index.row() < m_filters.size())
	{
		const FilterExpression& filter = m_filters[index.row()];
		switch (role)
		{
			case Qt::DisplayRole:
				data.setValue(filter.label());
				break;
			case Qt::UserRole:
				data.setValue(filter);
				break;
			case Qt::CheckStateRole:
				data.setValue(filter.enabled() ? Qt::Checked : Qt::Unchecked);
				break;
			case Qt::ForegroundRole:
				if (filter.isValid())
					data.setValue(QBrush(Qt::black));
				else
					data.setValue(QBrush(Qt::red));
		}
	}
	return data;
}


QStringList FilterModel::enabledExpressions() const
{
	QStringList result;
	for (const FilterExpression& filter : qAsConst(m_filters))
	{
		if (filter.enabled() && ! filter.isEmpty())
			result << filter.toString();
	}
	return result;
}


int FilterModel::findFilter(const FilterExpression& expr) const
{
	return m_filters.indexOf(expr);
}


int FilterModel::addFilter(const FilterExpression& expr)
{
	int changed = -1;
	int row = findFilter(expr);
	if (row >= 0)
	{
		changed = row;
		if (m_filters[row].enabled() != expr.enabled())
		{
			m_filters[row].setEnabled(expr.enabled());
			emit dataChanged(index(row), index(row));
		}
	}
	else
	{
		changed = m_filters.size();
		beginInsertRows(QModelIndex(), m_filters.size(), m_filters.size());
		m_filters << expr;
		endInsertRows();
	}

	return changed;
}


Qt::ItemFlags FilterModel::flags(const QModelIndex& index) const
{
	Qt::ItemFlags result = QAbstractListModel::flags(index);
	if (index.row() >= 0 && index.row() < m_filters.size())
		result |= Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
	return result;
}


bool FilterModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	bool ok = false;
	if (index.row() >= 0 && index.row() < m_filters.size())
	{
		switch (role)
		{
			case Qt::UserRole:
				m_filters[index.row()] = value.value<FilterExpression>();
				emit dataChanged(index, index);
				ok = true;
				break;
			case Qt::CheckStateRole:
				m_filters[index.row()].setEnabled(value.toInt() == Qt::Checked);
				ok = true;
				emit checkStateChanged(index);
				break;
			case Qt::DisplayRole:
				m_filters[index.row()].setLabel(value.toString());
				emit dataChanged(index, index);
				ok = true;
				break;
		}
	}

	return ok;
}


const QList<FilterExpression> & FilterModel::filters()
{
	return m_filters;
}


bool FilterModel::setAllEnabled(bool enabled)
{
	int first = -1, last = -1;
	for (int row=0; row < m_filters.size(); ++row)
	{
		FilterExpression& filter = m_filters[row];
		if (filter.enabled() != enabled)
		{
			filter.setEnabled(enabled);
			last = row;
			if (first < 0)
				first = row;
		}
	}
	if (first >= 0)
	{
		emit dataChanged(index(first), index(last));
		return true;
	}
	return false;
}


bool FilterModel::removeAllFilters()
{
	if (m_filters.size())
	{
		beginResetModel();
		m_filters.clear();
		endResetModel();
		return true;
	}
	return false;
}


bool FilterModel::invertFilter(const QModelIndex& index)
{
	if (index.row() >= 0 && index.row() < m_filters.size())
	{
		m_filters[index.row()] = m_filters[index.row()].inverted();
		emit dataChanged(index, index);
		return true;
	}
	return false;
}


bool FilterModel::removeRows(int row, int count, const QModelIndex& /*parent*/)
{
	if (row >= 0 && row < m_filters.size())
	{
		if (row + count > m_filters.size())
		{
			qDebug() << "removeRows: limiting count to m_filters.size(). requested:" << row << "+" << count << "max:" << m_filters.size();
			count = m_filters.size() - row;
		}
		if (count >= 0)
		{
			beginRemoveRows(QModelIndex(), row, row + count);
			auto it = m_filters.begin() + row;
			auto end = it + count;
			m_filters.erase(it, end);
			endRemoveRows();
			return true;
		}
	}
	return false;
}
