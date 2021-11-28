#include "savedviewsmodel.h"
#include "filtermodel.h"
#include "timespec.h"

#include <QDebug>

SavedViewsModel::SavedViewsModel(QObject* parent)
	: QAbstractTableModel(parent)
{
	QSettings settings;
	settings.beginGroup("views");
	m_views = SavedView::loadAll(settings);
}


int SavedViewsModel::columnCount(const QModelIndex& /*parent*/) const
{
	// name, columns, query, filters, time range, split
	return 6;
}


QVariant SavedViewsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		static const QStringList headers = {"Name", "Columns", "Query", "Filters", "Time Range", "Chart Splitting"};
		return headers[section];
	}
	return QAbstractTableModel::headerData(section, orientation, role);
}


int SavedViewsModel::rowCount(const QModelIndex& /*parent*/) const
{
	return m_views.size();
}


QVariant SavedViewsModel::data(const QModelIndex& index, int role) const
{
	if (role == Qt::DisplayRole)
	{
		const SavedView& v = m_views.at(index.row());
		switch (index.column())
		{
			case 0:
				return v.name();
			case 1:
				return v.columns().join(", ");
			case 2:
				if (v.hasQuery())
					return v.query();
				else
					return QVariant();
			case 3:
			{
				if (v.hasFilters())
				{
					QStringList filters;
					for (const FilterExpression& expr : v.filters())
					{
						if (expr.enabled())
							filters << expr.label();
					}
					return filters.join("\n") + " ";
				}
				return QVariant();
			}
			case 4:
			{
				if (v.hasTimerange())
					return QString("%1 to %2").arg(v.start().toString(), v.end().toString());
				return QVariant();
			}
			case 5:
			{
				QStringList result;
				quint32 limitBuckets = v.limitBuckets();
				if (v.hasSplitBy())
				{
					result << v.splitBy();
					if (limitBuckets)
						result << QString("Max Buckets: %1").arg(limitBuckets);
				}

				if (result.isEmpty())
					return QVariant();
				return result.join(", ");
			}
		}
	}
	return QVariant();
}


bool SavedViewsModel::removeRows(int row, int count, const QModelIndex& /*parent*/)
{
	beginRemoveRows(QModelIndex(), row, row + count -1);
	for (int i=0; i<count; ++i)
		m_views.removeAt(row);
	endRemoveRows();
	return true;
}


const SavedView& SavedViewsModel::itemAt(const QModelIndex& index) const
{
	return m_views[index.row()];
}


void SavedViewsModel::setItem(const QModelIndex& i, const SavedView& view)
{
	m_views[i.row()] = view;
	emit dataChanged(index(i.row(), 0), index(i.row(), columnCount(QModelIndex()) -1));
}
