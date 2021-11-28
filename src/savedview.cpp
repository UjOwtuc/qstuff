#include "savedview.h"
#include "filterexpression.h"

#include <QDebug>


QList<SavedView> SavedView::loadAll(QSettings& settings)
{
	QList<SavedView> views;
	QStringList names = settings.childGroups();
	for (const QString& name : qAsConst(names))
	{
		settings.beginGroup(name);
		SavedView view(name);
		view.load(settings);
		settings.endGroup();
		views << view;
	}
	return views;
}


void SavedView::saveAll(const QList<SavedView>& views, QSettings& settings)
{
	settings.remove("");
	for (const SavedView& view : views)
	{
		settings.beginGroup(view.m_name);
		view.save(settings);
		settings.endGroup();
	}
}


SavedView::SavedView(const QString& name)
	: m_name(name)
{
	m_hasColumns = false;
	m_hasQuery = false;
	m_hasFilters = false;
	m_hasTimerange = false;
	m_hasSplitBy = false;
}


void SavedView::load(QSettings& settings)
{
	m_hasColumns = settings.contains("columns");
	m_columns = settings.value("columns").toStringList();

	QVariant query = settings.value("query");
	if (query.isNull())
		m_hasQuery = false;
	m_query = query.toString();

	m_hasFilters = settings.contains("filters/size");
	m_filters = loadFiltersArray(settings);

	QVariant start = settings.value("start");
	QVariant end = settings.value("end");
	if (start.isNull() || end.isNull())
		m_hasTimerange = false;
	else
	{
		m_start = TimeSpec::deserialize(start.toStringList());
		m_end = TimeSpec::deserialize(end.toStringList());
		m_hasTimerange = true;
	}

	m_hasSplitBy = settings.contains("split_by");
	m_splitBy = settings.value("split_by").toString();
	m_limitBuckets = settings.value("limit_buckets", 5).toUInt();
}


void SavedView::save(QSettings& settings) const
{
	settings.remove("");
	settings.setValue("name", m_name);

	if (m_hasColumns)
		settings.setValue("columns", m_columns);

	if (m_hasQuery)
		settings.setValue("query", m_query);

	if (m_hasFilters)
		saveFiltersArray(settings, m_filters);

	if (m_hasTimerange)
	{
		settings.setValue("start", m_start.serialize());
		settings.setValue("end", m_end.serialize());
	}

	if (m_hasSplitBy)
	{
		settings.setValue("split_by", m_splitBy);
		settings.setValue("limit_buckets", m_limitBuckets);
	}
}
