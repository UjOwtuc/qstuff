#ifndef SAVEDVIEW_H
#define SAVEDVIEW_H

#include "filterexpression.h"
#include "timespec.h"

#include <QVariant>
#include <QSettings>

class SavedView
{
public:
	static QList<SavedView> loadAll(QSettings& settings);
	static void saveAll(const QList<SavedView>& views, QSettings& settings);

	explicit SavedView(const QString& name);

	void load(QSettings& settings);
	void save(QSettings& settings) const;

	const QString& name() const { return m_name; }
	void setName(const QString& set) { m_name = set; }

	bool hasColumns() const { return m_hasColumns; }
	void removeColumns() { m_hasColumns = false; }
	const QStringList& columns() const { return m_columns; }
	void setColumns(const QStringList& set) { m_columns = set; m_hasColumns = true; }

	bool hasQuery() const { return m_hasQuery; }
	void removeQuery() { m_hasQuery = false; }
	const QString& query() const { return m_query; }
	void setQuery(const QString& query) { m_query = query; m_hasQuery = true; }

	bool hasFilters() const { return m_hasFilters; }
	void removeFilters() { m_hasFilters = false; }
	const QList<FilterExpression> filters() const { return m_filters; }
	void setFilters(const QList<FilterExpression>& set) { m_filters = set; m_hasFilters = true; }

	bool hasTimerange() const { return m_hasTimerange; }
	void removeTimerange() { m_hasTimerange = false; }
	const TimeSpec& start() const { return m_start; }
	void setStart(const TimeSpec& set) { m_start = set; m_hasTimerange = true; }
	const TimeSpec& end() const { return m_end; }
	void setEnd(const TimeSpec& set) { m_end = set; m_hasTimerange = true; }
	void setTimerange(const TimeSpec& start, const TimeSpec& end) { m_start = start; m_end = end; m_hasTimerange = true; }

	bool hasSplitBy() const { return m_hasSplitBy; }
	void removeSplitBy() { m_hasSplitBy = false; }
	const QString& splitBy() const { return m_splitBy; }
	void setSplitBy(const QString& split) { m_splitBy = split; m_hasSplitBy = true; }
	quint32 limitBuckets() const { return m_limitBuckets; }
	void setLimitBuckets(quint32 set) { m_limitBuckets = set; m_hasSplitBy = true; }

private:
	QString m_name;

	bool m_hasColumns;
	QStringList m_columns;

	bool m_hasQuery;
	QString m_query;

	bool m_hasFilters;
	QList<FilterExpression> m_filters;

	bool m_hasTimerange;
	TimeSpec m_start;
	TimeSpec m_end;

	bool m_hasSplitBy;
	QString m_splitBy;
	quint32 m_limitBuckets;
};

#endif // SAVEDVIEW_H
