#ifndef SAVEDVIEWSMODEL_H
#define SAVEDVIEWSMODEL_H

#include "timespec.h"

#include <QAbstractTableModel>
#include <QSettings>

class FilterExpression;
class TimeSpec;

struct SavedView
{
	// required
	QString name;
	QStringList columns;

	// optional
	QVariant query;
	bool saveFilters;
	QList<FilterExpression> filters;
	bool saveTimerange;
	TimeSpec start;
	TimeSpec end;
	QVariant splitBy;
	QVariant limitBuckets;

};

class SavedViewsModel : public QAbstractTableModel
{
public:
	explicit SavedViewsModel(QObject* parent = nullptr);

	int columnCount(const QModelIndex& /*parent*/) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	int rowCount(const QModelIndex& /*parent*/) const override;
	QVariant data(const QModelIndex& index, int role) const override;
	bool removeRows(int row, int count, const QModelIndex& /*parent*/) override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role) override;

	void saveToSettings() const;

private:
	QList<SavedView> m_views;
};

#endif // SAVEDVIEWSMODEL_H
