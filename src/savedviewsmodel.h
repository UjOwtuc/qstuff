#ifndef SAVEDVIEWSMODEL_H
#define SAVEDVIEWSMODEL_H

#include "savedview.h"
#include "timespec.h"

#include <QAbstractTableModel>
#include <QSettings>

class FilterExpression;
class TimeSpec;

class SavedViewsModel : public QAbstractTableModel
{
public:
	explicit SavedViewsModel(QObject* parent = nullptr);

	int columnCount(const QModelIndex& /*parent*/) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	int rowCount(const QModelIndex& /*parent*/) const override;
	QVariant data(const QModelIndex& index, int role) const override;
	bool removeRows(int row, int count, const QModelIndex& /*parent*/) override;

	const QList<SavedView>& views() const { return m_views; }
	const SavedView& itemAt(const QModelIndex& index) const;
	void setItem(const QModelIndex& i, const SavedView& view);

private:
	QList<SavedView> m_views;
};

#endif // SAVEDVIEWSMODEL_H
