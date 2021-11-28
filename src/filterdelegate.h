#ifndef FILTERDELEGATE_H
#define FILTERDELEGATE_H

#include <QStyledItemDelegate>

class FilterDelegate : public QStyledItemDelegate
{
public:
	explicit FilterDelegate(QAbstractItemModel* editorCompletionsModel = nullptr, QObject* parent = nullptr);

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
	QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	void setEditorData(QWidget* editor, const QModelIndex& index) const;
	void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;

private:
	QAbstractItemModel* m_editorCompletionsModel;
};

#endif // FILTERDELEGATE_H
