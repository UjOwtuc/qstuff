#include "keyfilterproxymodel.h"


KeyFilterProxyModel::KeyFilterProxyModel(QObject* parent)
	: QSortFilterProxyModel(parent)
{}


bool KeyFilterProxyModel::filterAcceptsRow ( int row, const QModelIndex& parent ) const
{
	if (parent.isValid())
		return true;
	return QSortFilterProxyModel::filterAcceptsRow(row, parent);
}

