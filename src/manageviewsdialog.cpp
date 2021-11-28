#include "manageviewsdialog.h"
#include "savedviewsmodel.h"
#include "saveviewdialog.h"

#include <QStyledItemDelegate>
#include <QApplication>
#include <QDebug>

#include "ui_manageviewsdialog.h"


class OptionalSavedValueDelegate : public QStyledItemDelegate
{
public:
	explicit OptionalSavedValueDelegate(QObject* parent = nullptr)
		: QStyledItemDelegate(parent)
	{}

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		QVariant value = index.data();
		if (value.isNull())
		{
			QStyleOptionViewItem opt = option;
			initStyleOption(&opt, index);
			opt.text = "(not saved)";
			opt.palette.setColor(QPalette::Text, Qt::gray);
			QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, option.widget);
		}
		else
			QStyledItemDelegate::paint(painter, option, index);
	}

	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		if (index.data().isNull())
		{
			QSize content = option.fontMetrics.boundingRect(option.rect, Qt::TextSingleLine, "(not saved)").size();
			QSize padding = QApplication::style()->sizeFromContents(QStyle::CT_ItemViewItem, &option, QSize(), option.widget);
			QSize realSize = QSize(content.width() + padding.width(), std::max(content.height(), padding.height()));
			return realSize;
		}
		return QStyledItemDelegate::sizeHint(option, index);
	}
};

ManageViewsDialog::ManageViewsDialog(QWidget* parent, Qt::WindowFlags f)
	: QDialog(parent, f)
{
	m_widget = new Ui::ManageViewsDialog;
	m_widget->setupUi(this);

	m_viewsModel = new SavedViewsModel(this);
	m_widget->savedViewsTable->setModel(m_viewsModel);
	m_widget->savedViewsTable->setItemDelegate(new OptionalSavedValueDelegate(this));
	m_widget->savedViewsTable->resizeColumnsToContents();
	m_widget->savedViewsTable->resizeRowsToContents();

	connect(m_widget->savedViewsTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]{
		QModelIndexList selected = m_widget->savedViewsTable->selectionModel()->selectedIndexes();
		bool enable = !selected.isEmpty();
		m_widget->editViewButton->setEnabled(enable);
		m_widget->deleteViewButton->setEnabled(enable);
	});

	connect(m_widget->editViewButton, &QPushButton::clicked, this, &ManageViewsDialog::editView);
	connect(m_widget->deleteViewButton, &QPushButton::clicked, this, &ManageViewsDialog::removeViews);
	connect(m_widget->savedViewsTable, &QAbstractItemView::doubleClicked, this, &ManageViewsDialog::editView);
}


void ManageViewsDialog::removeViews()
{
	QModelIndexList selected = m_widget->savedViewsTable->selectionModel()->selectedIndexes();
	QSet<int> rows;
	for (const QModelIndex& index : qAsConst(selected))
		rows.insert(index.row());

	QList<int> sortable = rows.values();
	std::sort(sortable.rbegin(), sortable.rend());
	for (int row : qAsConst(sortable))
		m_viewsModel->removeRow(row);
}


void ManageViewsDialog::editView()
{
	QModelIndex current = m_widget->savedViewsTable->currentIndex();
	if (current.isValid())
	{
		SaveViewDialog dlg(this);
		dlg.setView(m_viewsModel->itemAt(current));
		if (dlg.exec() == QDialog::Accepted)
			m_viewsModel->setItem(current, dlg.view());
	}
}
