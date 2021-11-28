#include "savedviewsmodel.h"
#include "filtermodel.h"
#include "timespec.h"

#include <QDebug>

SavedViewsModel::SavedViewsModel(QObject* parent)
	: QAbstractTableModel(parent)
{
	QSettings settings;
	settings.beginGroup("views");
	QStringList names = settings.childGroups();

	for (const QString& name : qAsConst(names))
	{
		settings.beginGroup(name);
		SavedView view;
		view.name = name;
		view.columns = settings.value("columns").toStringList();
		view.query = settings.value("query");
		view.filters = loadFiltersArray(settings);
		view.saveFilters = settings.value("save_filters", !view.filters.isEmpty()).toBool();

		QVariant start = settings.value("start");
		QVariant end = settings.value("end");
		if (start.isNull() || end.isNull())
			view.saveTimerange = false;
		else
		{
			view.saveTimerange = true;
			view.start = TimeSpec::deserialize(start.toStringList());
			view.end = TimeSpec::deserialize(end.toStringList());
		}
		view.splitBy = settings.value("split_by");
		view.limitBuckets = settings.value("limit_buckets");
		settings.endGroup();
		m_views << view;
	}
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
				return v.name;
			case 1:
				return v.columns.join(", ");
			case 2:
				return v.query;
			case 3:
			{
				if (v.saveFilters)
				{
					QStringList filters;
					for (const FilterExpression& expr : qAsConst(v.filters))
						filters << expr.label();
					return filters.join(" and ");
				}
				return QVariant();
			}
			case 4:
			{
				if (v.saveTimerange)
					return QString("%1 to %2").arg(v.start.toString(), v.end.toString());
				return QVariant();
			}
			case 5:
			{
				QStringList result;
				quint32 limitBuckets = v.limitBuckets.toUInt();
				if (! v.splitBy.isNull())
					result << v.splitBy.toString();
				if (limitBuckets)
					result << QString("Max Buckets: %1").arg(limitBuckets);

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


Qt::ItemFlags SavedViewsModel::flags(const QModelIndex& index) const
{
	Qt::ItemFlags f = QAbstractTableModel::flags(index);
	if (index.column() == 0 || index.column() == 2)
		f |= Qt::ItemIsEditable;

	return f;
}


bool SavedViewsModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (role == Qt::EditRole)
	{
		bool changed = true;
		int row = index.row();
		switch (index.column())
		{
			case 0:
				m_views[row].name = value.toString();
				break;
			case 2:
				m_views[row].query = value;
				break;
			default:
				changed = false;
		}
		return changed;
	}
	return false;
}


void SavedViewsModel::saveToSettings() const
{
	QSettings settings;
	settings.beginGroup("views");

	// remove all saved views
	settings.remove("");

	for (const SavedView& view : qAsConst(m_views))
	{
		settings.beginGroup(view.name);
		settings.setValue("columns", view.columns);

		if (! view.query.isNull())
			settings.setValue("query", view.query);

		if (view.saveFilters)
			saveFiltersArray(settings, view.filters);

		if (view.saveTimerange)
		{
			settings.setValue("start", view.start.serialize());
			settings.setValue("end", view.end.serialize());
		}

		if (! view.splitBy.isNull())
			settings.setValue("split_by", view.splitBy);

		if (! view.limitBuckets.isNull())
			settings.setValue("limit_buckets", view.limitBuckets);

		settings.endGroup();
	}
}
