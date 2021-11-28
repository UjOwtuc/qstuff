#ifndef FILTERMODEL_H
#define FILTERMODEL_H

#include "filterexpression.h"

#include <QAbstractListModel>


class FilterModel : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit FilterModel(QObject* parent = nullptr);

	int rowCount(const QModelIndex & parent) const;
	QVariant data(const QModelIndex & index, int role) const;

	QStringList enabledExpressions() const;
	int findFilter(const FilterExpression& expr) const;
	int addFilter(const FilterExpression& expr);
	Qt::ItemFlags flags(const QModelIndex& index) const;
	bool setData(const QModelIndex & index, const QVariant & value, int role);
	const QList<FilterExpression>& filters();
	bool setAllEnabled(bool enabled);
	bool removeAllFilters();
	bool invertFilter(const QModelIndex& index);
	bool removeRows(int row, int count, const QModelIndex& /*parent*/);
	void setFiltersEditable(bool set) { m_filtersEditable = set; }

signals:
	void checkStateChanged(const QModelIndex& index);

private:
	QList<FilterExpression> m_filters;
	bool m_filtersEditable;
};


#endif // FILTERMODEL_H
