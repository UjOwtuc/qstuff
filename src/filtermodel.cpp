#include "filtermodel.h"

#include <QDebug>

namespace {
	QMap<FilterExpression::Op, QString> opStrings;

	void fillGlobalMaps()
	{
		opStrings.insert(FilterExpression::Eq, "=");
		opStrings.insert(FilterExpression::Neq, "!=");
		opStrings.insert(FilterExpression::Lt, "<");
		opStrings.insert(FilterExpression::Le, "<=");
		opStrings.insert(FilterExpression::Gt, ">");
		opStrings.insert(FilterExpression::Ge, ">=");
		opStrings.insert(FilterExpression::In, "in");
		opStrings.insert(FilterExpression::NotIn, "not in");
		opStrings.insert(FilterExpression::Like, "like");
		opStrings.insert(FilterExpression::NotLike, "not like");
	}
}


const QMap<FilterExpression::Op, QString>& FilterExpression::ops()
{
	if (opStrings.isEmpty())
		fillGlobalMaps();
	return opStrings;
}


QString FilterExpression::opString(FilterExpression::Op op)
{
	if (opStrings.isEmpty())
		fillGlobalMaps();
	return opStrings[op];
}


FilterExpression::Op FilterExpression::invertOp(FilterExpression::Op op)
{
	switch (op)
	{
		case Eq: return Neq;
		case Neq: return Eq;
		case Lt: return Ge;
		case Le: return Gt;
		case Gt: return Le;
		case Ge: return Lt;
		case In: return NotIn;
		case NotIn: return In;
		case Like: return NotLike;
		case NotLike: return Like;
	}
	qFatal("unhandled op");
}


FilterExpression::FilterExpression()
{
	m_enabled = true;
}


FilterExpression::FilterExpression(const FilterExpression& other)
{
	*this = other;
}


FilterExpression::FilterExpression(const QString& id, FilterExpression::Op op, const QString& value)
	: m_id(id), m_op(op), m_value(value), m_enabled(true)
{}


FilterExpression& FilterExpression::operator=(const FilterExpression& rhs)
{
	m_id = rhs.m_id;
	m_op = rhs.m_op;
	m_value = rhs.m_value;
	m_enabled = rhs.m_enabled;
	m_label = rhs.m_label;
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
	return QString("%1 %2 %3").arg(m_id).arg(opString(m_op)).arg(m_value);
}

FilterExpression FilterExpression::inverted() const
{
	FilterExpression inv(*this);
	inv.m_op = invertOp(m_op);
	if (! m_label.isEmpty())
		inv.m_label = QString("NOT %1").arg(m_label);

	return inv;
}


bool FilterExpression::enabled() const
{
	return m_enabled;
}


void FilterExpression::setEnabled(bool enabled)
{
	m_enabled = enabled;
}


void FilterExpression::setLabel(const QString& label)
{
	m_label = label;
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
		}
	}
	return data;
}


QStringList FilterModel::enabledExpressions() const
{
	QStringList result;
	for (auto filter : m_filters)
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
		if( m_filters[row].enabled() != expr.enabled())
		{
			m_filters[row].setEnabled(expr.enabled());
			dataChanged(index(row), index(row));
		}
	}
	else
	{
		row = findFilter(expr.inverted());
		if (row >= 0)
		{
			m_filters[row] = expr;
			auto modelIndex = index(row);
			dataChanged(modelIndex, modelIndex);
			changed = row;
		}
		else
		{
			changed = m_filters.size();
			beginInsertRows(QModelIndex(), m_filters.size(), m_filters.size());
			m_filters << expr;
			endInsertRows();
		}
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
				dataChanged(index, index);
				ok = true;
				break;
			case Qt::CheckStateRole:
				m_filters[index.row()].setEnabled(value.toInt() == Qt::Checked);
				ok = true;
				emit checkStateChanged(index);
				break;
			case Qt::DisplayRole:
				m_filters[index.row()].setLabel(value.toString());
				dataChanged(index, index);
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
		dataChanged(index(first), index(last));
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
		dataChanged(index, index);
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
