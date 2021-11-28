#include "filterlistwidget.h"
#include "filtermodel.h"
#include "filterdelegate.h"

#include <QAbstractItemDelegate>
#include <QSettings>
#include <QDebug>

#include "ui_filterlistwidget.h"


FilterListWidget::FilterListWidget(QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f)
{
	setUp();
	m_widget->filterList->setItemDelegate(new FilterDelegate(nullptr, this));
}


FilterListWidget::FilterListWidget(QAbstractItemModel* editorCompletionModel, QWidget* parent, Qt::WindowFlags f)

{
	setUp();
	m_widget->filterList->setItemDelegate(new FilterDelegate(editorCompletionModel, this));
}


void FilterListWidget::setUp()
{
	m_widget = new Ui::FilterListWidget;
	m_widget->setupUi(this);

	m_filterModel = new FilterModel(this);
	m_widget->filterList->setModel(m_filterModel);

	connect(m_widget->filterList->itemDelegate(), &QAbstractItemDelegate::commitData, this, [this]{
		if (checkFiltersChanged())
			emit filtersChanged();
	});
	connect(m_filterModel, &FilterModel::checkStateChanged, this, &FilterListWidget::filtersChanged);
	connect(m_widget->addFilterButton, &QToolButton::clicked, this, [this]{
		int row = m_filterModel->addFilter(FilterExpression());
		m_widget->filterList->edit(m_filterModel->index(row));
	});
	connect(m_widget->enableAllFiltersButton, &QToolButton::clicked, this, [this]{
		if(m_filterModel->setAllEnabled(true))
			emit filtersChanged();
	});
	connect(m_widget->disableAllFiltersButton, &QToolButton::clicked, this, [this]{
		if (m_filterModel->setAllEnabled(false))
			emit filtersChanged();
	});
	connect(m_widget->removeAllFiltersButton, &QToolButton::clicked, this, &FilterListWidget::removeAll);
	connect(m_widget->invertFilterButton, &QToolButton::clicked, this, &FilterListWidget::invertSelected);
	connect(m_widget->removeFilterButton, &QToolButton::clicked, this, &FilterListWidget::removeSelected);

	connect(m_widget->filterList->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]{
		QModelIndexList selected = m_widget->filterList->selectionModel()->selectedRows();
		bool enable_buttons = ! selected.isEmpty();
		m_widget->invertFilterButton->setEnabled(enable_buttons);
		m_widget->removeFilterButton->setEnabled(enable_buttons);
	});
	connect(this, &FilterListWidget::filtersChanged, this, [this]{
		m_lastFilterState = combinedFilterString();
	});
}


const QList<FilterExpression>& FilterListWidget::filters() const
{
	return m_filterModel->filters();
}


QString FilterListWidget::combinedFilterString() const
{
	 return m_filterModel->enabledExpressions().join(" and ");
}


void FilterListWidget::loadFilters()
{
	QSettings settings;
	QList<FilterExpression> filters = loadFiltersArray(settings);
	for (const FilterExpression& filter : qAsConst(filters))
		m_filterModel->addFilter(filter);
}


void FilterListWidget::saveFilters() const
{
	QSettings settings;
	QList<FilterExpression> filters = m_filterModel->filters();
	filters.removeAll(FilterExpression());
	saveFiltersArray(settings, filters);
}


int FilterListWidget::addFilter(const FilterExpression& expr)
{
	return m_filterModel->addFilter(expr);
}


void FilterListWidget::setFilters(const QList<FilterExpression>& filters)
{
	m_filterModel->setAllEnabled(false);
	for (const FilterExpression& filter : qAsConst(filters))
		m_filterModel->addFilter(filter);
}


void FilterListWidget::setFiltersEditable(bool set)
{
	m_widget->addFilterButton->setEnabled(set);
	m_filterModel->setFiltersEditable(set);
}


void FilterListWidget::invertSelected()
{
	QModelIndexList selected = m_widget->filterList->selectionModel()->selectedRows();
	for (const QModelIndex& index : qAsConst(selected))
		m_filterModel->invertFilter(index);
	if (checkFiltersChanged())
		emit filtersChanged();
}


void FilterListWidget::removeSelected()
{
	QModelIndexList selected = m_widget->filterList->selectionModel()->selectedRows();
	// sort by row descending, so removing by row numer works
	std::sort(selected.begin(), selected.end(), [](QModelIndex a, QModelIndex b) {
		return a.row() > b.row();
	});
	for (auto index : qAsConst(selected))
		m_filterModel->removeRow(index.row());

	if (checkFiltersChanged())
		emit filtersChanged();
}


void FilterListWidget::removeAll()
{
	if (m_filterModel->filters().size())
	{
		m_filterModel->removeAllFilters();
		emit filtersChanged();
	}
}


bool FilterListWidget::checkFiltersChanged()
{
	return m_lastFilterState != combinedFilterString();
}
