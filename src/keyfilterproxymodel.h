#ifndef KEYFILTERPROXYMODEL_H
#define KEYFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

class KeyFilterProxyModel : public QSortFilterProxyModel
{
public:
	explicit KeyFilterProxyModel(QObject* parent = nullptr);

	bool filterAcceptsRow (int row, const QModelIndex& parent) const;
};

#endif // KEYFILTERPROXYMODEL_H
