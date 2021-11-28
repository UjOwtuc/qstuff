#ifndef SAVEVIEWDIALOG_H
#define SAVEVIEWDIALOG_H

#include "filterexpression.h"
#include "timespec.h"
#include "savedview.h"

#include <QDialog>

namespace Ui {
	class SaveViewDialog;
}
class QStringListModel;
class TimerangeModel;

class SaveViewDialog : public QDialog
{
public:
	explicit SaveViewDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

	QString name() const;
	void setName(const QString& name);

	bool saveColumns() const;
	void setSaveColumns(bool save);
	QStringList columns() const;
	void setColumns(const QStringList& columns);

	bool saveQuery() const;
	void setSaveQuery(bool save);
	QString query() const;
	void setQuery(const QString& value);

	bool saveFilters() const;
	void setSaveFilters(bool save);
	QList<FilterExpression> filters() const;
	void setFilters(const QList<FilterExpression>& filters);

	bool saveTimerange() const;
	void setSaveTimerange(bool save);
	TimeSpec start() const;
	TimeSpec end() const;
	void setTimerange(const TimeSpec& start, const TimeSpec& end);

	bool saveSplitBy() const;
	void setSaveSplitBy(bool save);
	QString splitBy() const;
	void setSplitBy(const QString& splitBy);
	quint32 limitBuckets() const;
	void setLimitBuckets(quint32 limit);

	SavedView view() const;
	void setView(const SavedView& view);

	void moveColumns(bool down);

private:
	Ui::SaveViewDialog* m_widget;

	QStringListModel* m_columnsModel;
	TimerangeModel* m_timerangeModel;
};

#endif // SAVEVIEWDIALOG_H
