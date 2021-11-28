#include "filterdelegate.h"
#include "editfilterwidget.h"
#include "filterexpression.h"

#include <QApplication>
#include <QDebug>


FilterDelegate::FilterDelegate(QAbstractItemModel* editorCompletionsModel, QObject* parent)
	: QStyledItemDelegate(parent),
	m_editorCompletionsModel(editorCompletionsModel)
{}


void FilterDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	FilterExpression filter = index.data(Qt::UserRole).value<FilterExpression>();
	QStyleOptionViewItem opt(option);
	initStyleOption(&opt, index);
	opt.text = filter.label();
	if (filter.isEmpty())
		opt.palette.setColor(QPalette::Text, Qt::red);

	QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, option.widget);
}


QSize FilterDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	if (option.state == QStyle::State_Editing)
		return QSize(800, 400);
	return QStyledItemDelegate::sizeHint(option, index);
}


QWidget* FilterDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	EditFilterWidget* editor = new EditFilterWidget(parent);
	editor->setExpression(index.data(Qt::UserRole).value<FilterExpression>());
	editor->setAutoFillBackground(true);
	editor->setBackgroundRole(QPalette::Window);

	if (m_editorCompletionsModel)
		editor->setCompletionsModel(m_editorCompletionsModel);

	return editor;
}


void FilterDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	editor->setProperty("expression", index.data(Qt::UserRole));
}


void FilterDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
	QVariant data = editor->property("expression");
	model->setData(index, data, Qt::UserRole);
	QVariant label = editor->property("label");
	model->setData(index, label, Qt::DisplayRole);
}
