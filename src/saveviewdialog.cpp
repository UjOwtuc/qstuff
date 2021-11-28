#include "saveviewdialog.h"
#include "timerangemodel.h"

#include <QStringListModel>
#include <QDebug>

#include "ui_saveviewdialog.h"

SaveViewDialog::SaveViewDialog(QWidget* parent, Qt::WindowFlags f)
	: QDialog(parent, f)
{
	m_widget = new Ui::SaveViewDialog;
	m_widget->setupUi(this);

	m_widget->filterListWidget->setFiltersEditable(false);

	m_columnsModel = new QStringListModel(this);
	m_widget->columnsList->setModel(m_columnsModel);

	m_timerangeModel = new TimerangeModel(this);
	m_widget->timerangeCombo->setModel(m_timerangeModel);

	connect(m_widget->columnsList->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]{
		QModelIndexList selected = m_widget->columnsList->selectionModel()->selectedIndexes();
		bool enable = !selected.isEmpty();
		m_widget->columnUpButton->setEnabled(enable);
		m_widget->columnDownButton->setEnabled(enable);
		m_widget->removeColumnButton->setEnabled(enable);
	});

	connect(m_widget->columnUpButton, &QPushButton::clicked, this, [this]{
		moveColumns(false);
	});
	connect(m_widget->columnDownButton, &QPushButton::clicked, this, [this]{
		moveColumns(true);
	});
}


QString SaveViewDialog::name() const
{
	return m_widget->nameEdit->text();
}


void SaveViewDialog::setName(const QString& name)
{
	m_widget->nameEdit->setText(name);
}


bool SaveViewDialog::saveColumns() const
{
	return m_widget->columnsGroup->isChecked();
}


void SaveViewDialog::setSaveColumns(bool save)
{
	m_widget->columnsGroup->setChecked(save);
}


QStringList SaveViewDialog::columns() const
{
	return m_columnsModel->stringList();
}


void SaveViewDialog::setColumns(const QStringList& columns)
{
	m_columnsModel->setStringList(columns);
}


bool SaveViewDialog::saveQuery() const
{
	return m_widget->queryGroup->isChecked();
}


void SaveViewDialog::setSaveQuery(bool save)
{
	m_widget->queryGroup->setChecked(save);
}


QString SaveViewDialog::query() const
{
	return m_widget->queryCombo->currentText();
}


void SaveViewDialog::setQuery(const QString& value)
{
	m_widget->queryCombo->setCurrentText(value);
}


bool SaveViewDialog::saveFilters() const
{
	return m_widget->filtersGroup->isChecked();
}


void SaveViewDialog::setSaveFilters(bool save)
{
	m_widget->filtersGroup->setChecked(save);
}


QList<FilterExpression> SaveViewDialog::filters() const
{
	return m_widget->filterListWidget->filters();
}


void SaveViewDialog::setFilters(const QList<FilterExpression>& filters)
{
	m_widget->filterListWidget->setFilters(filters);
}


bool SaveViewDialog::saveTimerange() const
{
	return m_widget->timerangeBox->isChecked();
}


void SaveViewDialog::setSaveTimerange(bool save)
{
	m_widget->timerangeBox->setChecked(save);
}


TimeSpec SaveViewDialog::start() const
{
	auto timerange = m_widget->timerangeCombo->currentData(Qt::UserRole).value<QPair<TimeSpec, TimeSpec>>();
	return timerange.first;
}


TimeSpec SaveViewDialog::end() const
{
	auto timerange = m_widget->timerangeCombo->currentData(Qt::UserRole).value<QPair<TimeSpec, TimeSpec>>();
	return timerange.second;
}


void SaveViewDialog::setTimerange(const TimeSpec& start, const TimeSpec& end)
{
	int newRow = m_timerangeModel->addChoice(start, end);
	m_widget->timerangeCombo->setCurrentIndex(newRow);
}


bool SaveViewDialog::saveSplitBy() const
{
	return m_widget->chartSplittingBox->isChecked();
}


void SaveViewDialog::setSaveSplitBy(bool save)
{
	m_widget->chartSplittingBox->setChecked(save);
}


QString SaveViewDialog::splitBy() const
{
	return m_widget->splitByCombo->currentText();
}


void SaveViewDialog::setSplitBy(const QString& splitBy)
{
	m_widget->splitByCombo->setCurrentText(splitBy);
}


quint32 SaveViewDialog::limitBuckets() const
{
	return m_widget->limitBucketsSpinbox->value();
}


void SaveViewDialog::setLimitBuckets(quint32 limit)
{
	m_widget->limitBucketsSpinbox->setValue(limit);
}


SavedView SaveViewDialog::view() const
{
	SavedView view(name());
	if (saveColumns())
		view.setColumns(columns());
	if (saveQuery())
		view.setQuery(query());
	if (saveFilters())
		view.setFilters(filters());
	if (saveTimerange())
		view.setTimerange(start(), end());
	if (saveSplitBy())
	{
		view.setSplitBy(splitBy());
		view.setLimitBuckets(limitBuckets());
	}
	return view;
}


void SaveViewDialog::setView(const SavedView& view)
{
	setName(view.name());

	setSaveColumns(view.hasColumns());
	if (view.hasColumns())
		setColumns(view.columns());

	setSaveQuery(view.hasQuery());
	if (view.hasQuery())
		setQuery(view.query());

	setSaveFilters(view.hasFilters());
	if (view.hasFilters())
		setFilters(view.filters());

	setSaveTimerange(view.hasTimerange());
	if (view.hasTimerange())
		setTimerange(view.start(), view.end());

	setSaveSplitBy(view.hasSplitBy());
	if (view.hasSplitBy())
	{
		setSplitBy(view.splitBy());
		setLimitBuckets(view.limitBuckets());
	}
}


void SaveViewDialog::moveColumns(bool down)
{
	QModelIndexList selected = m_widget->columnsList->selectionModel()->selectedIndexes();
	for (const QModelIndex& index : qAsConst(selected))
	{
		int srcRow = index.row();
		int destRow = down ? srcRow +2 : srcRow -1;
		m_columnsModel->moveRows(QModelIndex(), srcRow, 1, QModelIndex(), destRow);
	}
}
