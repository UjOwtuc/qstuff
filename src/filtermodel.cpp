#include "filtermodel.h"
#include "filterexpression.h"
#include "queryvalidator.h"

#include <QBrush>
#include <QDebug>


FilterModel::FilterModel(QObject* parent)
	: QAbstractListModel(parent),
	m_filtersEditable(true)
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
	{
		result |= Qt::ItemIsUserCheckable;
		if (m_filtersEditable)
			result |= Qt::ItemIsEditable;
	}
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
