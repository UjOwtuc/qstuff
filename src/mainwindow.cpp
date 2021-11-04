#include "mainwindow.h"
#include "percentbardelegate.h"
#include "logmodel.h"
#include "timeinputdialog.h"
#include "timerangemodel.h"
#include "saveviewdialog.h"
#include "keyfilterproxymodel.h"
#include "countschart.h"
#include "filtermodel.h"
#include "filterdelegate.h"

#include <QLineEdit>
#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardItemModel>
#include <QTimer>
#include <QSettings>
#include <QSortFilterProxyModel>

#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QChart>

#include "ui_mainwindow.h"


static const char *clearLineXpm[]={
"15 5 2 1",
"# c #000000",
". c #ffffff",
".....###..#...#",
"..####.....#.#.",
"###.........#..",
"..####.....#.#.",
".....###..#...#"};


QString formatDuration(const QDateTime& start, const QDateTime& end)
{
	QStringList result;
	quint64 difference = start.secsTo(end);
	if (difference >= 3600 * 24)
	{
		quint64 days = difference / (3600 * 24);
		result << QString("%1 days").arg(days);
		difference %= 3600 * 24;
	}

	if (difference >= 3600 || (result.size() && difference > 0))
	{
		int hours = difference / 3600;
		result << QString("%1 hours").arg(hours);
		difference %= 3600;
	}

	if (difference >= 60 || (result.size() && difference > 0))
	{
		int minutes = difference / 60;
		result << QString("%1 minutes").arg(minutes);
		difference %= 60;
	}

	if (difference > 0 || result.isEmpty())
		result << QString("%1 seconds").arg(difference);

	return result.join(" ");
}


void saveFiltersArray(QSettings& settings, const QList<FilterExpression>& filters)
{
	int size = filters.size();
	settings.beginWriteArray("filters", size);
	for (int i=0; i<size; ++i)
	{
		settings.setArrayIndex(i);
		const FilterExpression& filter = filters.at(i);
		settings.setValue("id", filter.id());
		settings.setValue("op", filter.op());
		settings.setValue("value", filter.value());
		if (filter.label() != filter.toString())
			settings.setValue("label", filter.label());
		else
			settings.setValue("label", QString());
		settings.setValue("enabled", filter.enabled());
	}
	settings.endArray();
}


QList<FilterExpression> loadFiltersArray(QSettings& settings)
{
	QList<FilterExpression> filters;
	int size = settings.beginReadArray("filters");
	for (int i=0; i<size; ++i)
	{
		settings.setArrayIndex(i);
		QString id = settings.value("id").toString();
		FilterExpression::Op op = static_cast<FilterExpression::Op>(settings.value("op").toInt());
		QString value = settings.value("value").toString();
		QString label = settings.value("label").toString();
		bool enabled = settings.value("enabled").toBool();

		FilterExpression expr(id, op, value);
		expr.setLabel(label);
		expr.setEnabled(enabled);
		filters << expr;
	}
	settings.endArray();
	return filters;
}


QStuffMainWindow::QStuffMainWindow()
	: QMainWindow()
{
	m_widget = new Ui::QStuffMainWindow();
	m_widget->setupUi(this);
	m_netAccess = new QNetworkAccessManager(this);

	setupKeysView();
	setupFilterView();
	setupChartView();

	QAction* toggleLogDetails = m_widget->logDetailsDock->toggleViewAction();
	toggleLogDetails->setText("Show &Details");
	m_widget->menu_Window->addAction(toggleLogDetails);

	QSettings settings;
	restoreGeometry(settings.value("mainwindow/geometry").toByteArray());
	if (! restoreState(settings.value("mainwindow/windowState").toByteArray()))
		setupReasonableDockWidgetPositions();


	m_logModel = new LogModel(settings.value("default_columns", QStringList({"hostname", "programname", "msg"})).toStringList());
	m_widget->logsTable->setModel(m_logModel);

	m_timerangeModel = new TimerangeModel(this);
	m_widget->timerangeCombo->setModel(m_timerangeModel);

	QIcon clearIcon = QIcon::fromTheme("edit-clear", QIcon(QPixmap(clearLineXpm)));
	QAction* clearQuery = m_widget->queryInputCombo->lineEdit()->addAction(clearIcon, QLineEdit::TrailingPosition);;
	connect(clearQuery, &QAction::triggered, [this]{
		m_widget->queryInputCombo->lineEdit()->clear();
		search();
	});

	connect(m_widget->queryInputCombo->lineEdit(), &QLineEdit::returnPressed, this, &QStuffMainWindow::search);
	connect(m_netAccess, &QNetworkAccessManager::finished, this, &QStuffMainWindow::requestFinished);
	connect(m_widget->timerangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QStuffMainWindow::currentTimerangeChanged);

	connect(m_widget->logsTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, &QStuffMainWindow::currentLogItemChanged);

	QAction* refresh = new QAction(this);
	refresh->setShortcut(Qt::Key_F5);
	connect(refresh, &QAction::triggered, this, &QStuffMainWindow::search);
	addAction(refresh);

	connect(m_widget->action_saveView, &QAction::triggered, this, &QStuffMainWindow::saveView);
	connect(m_widget->action_resetView, &QAction::triggered, [this]{
		m_logModel->setColumns({"hostname", "programname", "msg"});
		m_widget->logsTable->resizeColumnsToContents();
	});

	settings.beginGroup("views");
	for (auto name : settings.childGroups())
	{
		QAction* loadViewAction = new QAction(name, this);
		connect(loadViewAction, &QAction::triggered, [this,name]{
			loadView(name);
		});
		m_widget->menu_view->addAction(loadViewAction);
	}
	settings.endGroup();

	loadTimerangeChoices();
	loadQueryHistory();

	QTimer::singleShot(0, this, &QStuffMainWindow::search);
}


void QStuffMainWindow::search()
{
	QUrlQuery queryItems;
	auto timerange = m_widget->timerangeCombo->currentData(Qt::UserRole).value<QPair<TimeSpec, TimeSpec>>();

	QDateTime start = timerange.first.toDateTime();
	QDateTime end = timerange.second.toDateTime();

// 	qDebug() << "search from" << start << "to" << end << "for" << m_widget->queryInputCombo->currentText();
	if (start.msecsTo(end) > 0)
	{
		if (focusWidget() == m_widget->timerangeCombo)
			m_lastInputFocus = Timerange;
		else if (focusWidget() == m_widget->queryInputCombo)
			m_lastInputFocus = Query;
		else
			m_lastInputFocus = Other;

		setInputsEnabled(false);
		m_widget->statusbar->clearMessage();

		QString query = m_filterModel->enabledExpressions().join(" and ");
		QString input = m_widget->queryInputCombo->currentText();
		if (! input.isEmpty())
		{
			if (query.isEmpty())
				query = input;
			else
				query = QString("(%1) and %2").arg(query).arg(input);
		}

		queryItems.addQueryItem("limit_events", "2000");
		queryItems.addQueryItem("start", start.toUTC().toString(Qt::ISODate));
		queryItems.addQueryItem("end", end.toUTC().toString(Qt::ISODate));
		queryItems.addQueryItem("query", query);
		QUrl url("http://localhost:8080/events");
		url.setQuery(queryItems);
		QNetworkRequest req(url);
		auto reply = m_netAccess->get(req);

		QLabel* taskLabel = new QLabel("Waiting for response headers", m_widget->statusbar);
		m_widget->statusbar->addWidget(taskLabel);

		QProgressBar* progress = new QProgressBar(m_widget->statusbar);
		progress->setRange(0, 0);
		m_widget->statusbar->addWidget(progress);

		connect(reply, &QNetworkReply::downloadProgress, [progress,taskLabel](quint64 received, quint64 total){
			progress->setMaximum(total);
			progress->setValue(received);
			taskLabel->setText("Receiving events");
		});
		connect(reply, &QNetworkReply::finished, progress, &QProgressBar::deleteLater);
		connect(reply, &QNetworkReply::finished, taskLabel, &QProgressBar::deleteLater);

		reply->setProperty("duration string", formatDuration(start, end));
	}
}


void QStuffMainWindow::requestFinished(QNetworkReply* reply)
{
	auto error = reply->error();
	if (error == QNetworkReply::NoError)
	{
		m_widget->detailsTable->clearContents();
		m_widget->detailsTable->setRowCount(0);
		QJsonParseError parseError;
		QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
		if (parseError.error == QJsonParseError::NoError)
		{
			QJsonObject obj(doc.object());
			QJsonObject topFields = obj["fields"].toObject();
			setKeys(topFields);

			QJsonArray events = obj["events"].toArray();
			m_logModel->setLogs(events.toVariantList());
			m_widget->logsTable->resizeColumnsToContents();

			QVariantMap metadata = obj["metadata"].toObject().toVariantMap();
			quint64 eventCount = metadata["event_count"].toULongLong();
			quint64 displayed = m_logModel->rowCount(QModelIndex());

			m_widget->statusbar->showMessage(
				QString("filter matched %1 of %2 estimated events in %3")
					.arg(displayed)
					.arg(eventCount)
					.arg(reply->property("duration string").toString())
			);
			QVariantMap counts = obj["counts"].toObject().toVariantMap();
			m_countsChart->plotCounts(counts);
			m_countsChart->setInterval(metadata["counts_interval_sec"].toInt());
		}
		else
			QMessageBox::warning(this, "Malformed Reply", QString("Could not parse server's reply: %1").arg(parseError.errorString()));
	}
	else
		QMessageBox::warning(this, "Network Error", QString("Network request failed: %1").arg(reply->errorString()));

	setInputsEnabled(true);
	switch (m_lastInputFocus)
	{
		case Query:
			m_widget->queryInputCombo->setFocus();
			break;
		case Timerange:
			m_widget->timerangeCombo->setFocus();
			break;
		case Other:
			break;
	}
}


void QStuffMainWindow::setKeys(const QJsonObject& keys)
{
	auto rootItem = m_keysModel->invisibleRootItem();
	QStringList expandedKeys;
	for (int row=0; row<rootItem->rowCount(); ++row)
	{
		if (m_widget->keysTree->isExpanded(m_keysProxy->mapFromSource(m_keysModel->index(row, 0, QModelIndex()))))
			expandedKeys << rootItem->child(row)->data(Qt::DisplayRole).toString();
	}

	rootItem->removeRows(0, rootItem->rowCount());

	auto keymap = keys.toVariantMap();
	auto end = keymap.end();
	int row = 0;
	for (auto it=keymap.begin(); it!=end; ++it, ++row)
	{
		QStandardItem* item = new QStandardItem(it.key());
		item->setData(it.key(), Qt::UserRole);

		QVariantMap values = it.value().toMap();
		auto valueEnd = values.constEnd();
		for (auto valueIt=values.constBegin(); valueIt!=valueEnd; ++valueIt)
		{
			QStandardItem* value = new QStandardItem(valueIt.key());
			QStandardItem* percentage = new QStandardItem(valueIt.value().toString());
			int percentValue = valueIt.value().toInt();
			value->setData(1000 - percentValue, Qt::UserRole);
			percentage->setData(percentValue, Qt::UserRole);
			item->appendRow({value, percentage});
		}
		rootItem->appendRow(item);

		if (expandedKeys.contains(it.key()))
			m_widget->keysTree->expand(m_keysProxy->mapFromSource(item->index()));
	}
	m_widget->keysTree->resizeColumnToContents(0);
	m_keysProxy->sort(0);
}


void QStuffMainWindow::closeEvent(QCloseEvent* event)
{
	saveQueryHistory();
	saveFilters();
	QSettings settings;
	settings.setValue("mainwindow/geometry", saveGeometry());
	settings.setValue("mainwindow/windowState", saveState());
	QMainWindow::closeEvent(event);
}


void QStuffMainWindow::saveQueryHistory()
{
	QSettings settings;
	settings.beginWriteArray("query_history", m_widget->queryInputCombo->count());
	for (int i=0; i<m_widget->queryInputCombo->count(); ++i)
	{
		settings.setArrayIndex(i);
		settings.setValue("query", m_widget->queryInputCombo->itemText(i));
	}
	settings.endArray();
}


void QStuffMainWindow::loadQueryHistory()
{
	QSignalBlocker blocker(m_widget->queryInputCombo);
	QSettings settings;
	int size = settings.beginReadArray("query_history");
	for (int i=0; i<size; ++i)
	{
		settings.setArrayIndex(i);
		m_widget->queryInputCombo->addItem(settings.value("query").toString());
	}
	settings.endArray();
}


void QStuffMainWindow::loadTimerangeChoices()
{
	QSignalBlocker blocker(m_widget->timerangeCombo);
	QSettings settings;
	int size = settings.beginReadArray("timerange_choices");
	for (int i=0; i<size; ++i)
	{
		settings.setArrayIndex(i);
		TimeSpec start = TimeSpec::deserialize(settings.value("start").toStringList());
		TimeSpec end = TimeSpec::deserialize(settings.value("end").toStringList());
		m_timerangeModel->addChoice(start, end);
	}
	settings.endArray();

	if (size < 1)
	{
		m_timerangeModel->addChoice(TimeSpec(15, TimeSpec::Minutes), TimeSpec());
		m_timerangeModel->addChoice(TimeSpec(1, TimeSpec::Hours), TimeSpec());
		m_timerangeModel->addChoice(TimeSpec(4, TimeSpec::Hours), TimeSpec());
		m_timerangeModel->addChoice(TimeSpec(1, TimeSpec::Days), TimeSpec());
		m_timerangeModel->addChoice(TimeSpec(1, TimeSpec::Weeks), TimeSpec());
		m_timerangeModel->addChoice(TimeSpec(1, TimeSpec::Months), TimeSpec());
		m_timerangeModel->addChoice(TimeSpec(1, TimeSpec::Years), TimeSpec());
	}
	m_widget->timerangeCombo->setCurrentIndex(0);
}


void QStuffMainWindow::saveFilters()
{
	QSettings settings;
	QList<FilterExpression> filters = m_filterModel->filters();
	filters.removeAll(FilterExpression());
	saveFiltersArray(settings, filters);
}


void QStuffMainWindow::currentLogItemChanged(const QItemSelection& selected, const QItemSelection& /* deselected */)
{
	m_widget->detailsTable->clearContents();
	if (! selected.indexes().isEmpty())
	{
		QModelIndex current = selected.indexes().first();
		auto data = m_logModel->rowData(current.row()).toMap();
		auto event = data["source"].toMap();

		m_widget->detailsTable->setColumnCount(2);
		m_widget->detailsTable->setRowCount(event.size() +1);

		m_widget->detailsTable->setItem(0, 0, new QTableWidgetItem("timestamp"));
		m_widget->detailsTable->setItem(0, 1, new QTableWidgetItem(data["timestamp"].toDateTime().toString()));

		auto end = event.constEnd();
		int row = 1;
		for (auto it=event.constBegin(); it!=end; ++it, ++row)
		{
			m_widget->detailsTable->setItem(row, 0, new QTableWidgetItem(it.key()));
			QString value;
			if (it.value().canConvert<QStringList>())
				value = it.value().toStringList().join(", ");
			else
				value = it.value().toString();
			m_widget->detailsTable->setItem(row, 1, new QTableWidgetItem(value));
		}
		m_widget->detailsTable->resizeColumnsToContents();
		m_widget->detailsTable->resizeRowsToContents();
		m_widget->logDetailsDock->show();
	}
}


void QStuffMainWindow::currentTimerangeChanged(int current)
{
	QVariant data = m_timerangeModel->data(m_timerangeModel->index(current, 0), Qt::UserRole);
	if (! data.isValid())
	{
		TimeInputDialog dialog(this);
		if (dialog.exec() == QDialog::Accepted)
		{
			m_widget->timerangeCombo->blockSignals(true);
			int newRow = m_timerangeModel->addChoice(dialog.startTime(), dialog.endTime());
			m_widget->timerangeCombo->setCurrentIndex(newRow);
			m_widget->timerangeCombo->blockSignals(false);
		}
	}
	search();
}


void QStuffMainWindow::toggleKeyColumn(int keyIndex)
{
	QString keyName = m_keysModel->item(keyIndex, 0)->text();
	m_logModel->toggleColumn(keyName);
}


void QStuffMainWindow::showKeysContextMenu(const QPoint& point)
{
	QModelIndex index = m_keysProxy->mapToSource(m_widget->keysTree->indexAt(point));
	if (index.isValid())
	{
		QMenu* contextMenu = new QMenu(this);
		if (m_keysModel->parent(index) == QModelIndex())
		{
			QString key = m_keysModel->data(m_keysModel->index(index.row(), 0, index.parent()), Qt::DisplayRole).toString();
			QAction* toggle = new QAction("Toggle column in log view", contextMenu);
			connect(toggle, &QAction::triggered, [this, key]{
				m_logModel->toggleColumn(key);
				m_widget->logsTable->resizeColumnsToContents();
			});
			contextMenu->addAction(toggle);
		}
		else
		{
			QString key = m_keysModel->data(index.parent(), Qt::DisplayRole).toString();
			QString value = m_keysModel->data(m_keysModel->index(index.row(), 0, index.parent()), Qt::DisplayRole).toString();

			// TODO: escape quotes in value
			value = QString("\"%1\"").arg(value);
			QAction* filter = new QAction("Filter for value", contextMenu);
			connect(filter, &QAction::triggered, [this, key, value]{
				if (m_filterModel->addFilter(FilterExpression(key, FilterExpression::Eq, value)) >= 0)
					search();
			});
			contextMenu->addAction(filter);

			QAction* filterNot = new QAction("Filter out value", contextMenu);
			connect(filterNot, &QAction::triggered, [this, key, value]{
				if (m_filterModel->addFilter(FilterExpression(key, FilterExpression::Neq, value)) >= 0)
					search();
			});
			contextMenu->addAction(filterNot);
		}
		contextMenu->exec(m_widget->keysTree->viewport()->mapToGlobal(point));
	}
}


void QStuffMainWindow::loadView(const QString& name)
{
	QSettings settings;
	settings.beginGroup("views");
	settings.beginGroup(name);

	const QSignalBlocker queryBlocker(m_widget->queryInputCombo);
	const QSignalBlocker timeBlocker(m_widget->timerangeCombo);

	m_widget->queryInputCombo->setCurrentText(settings.value("query").toString());
	QVariant columns = settings.value("columns");
	if (! columns.isNull())
		m_logModel->setColumns(columns.toStringList());

	QVariant start = settings.value("start");
	QVariant end = settings.value("end");
	if (! start.isNull() && ! end.isNull())
	{
		int index = m_timerangeModel->addChoice(TimeSpec::deserialize(start.toStringList()), TimeSpec::deserialize(end.toStringList()));
		m_widget->timerangeCombo->setCurrentIndex(index);
	}

	QList<FilterExpression> filters = loadFiltersArray(settings);
	m_filterModel->setAllEnabled(false);
	for (auto filter : filters)
		m_filterModel->addFilter(filter);

	search();
}


void QStuffMainWindow::saveView()
{
	SaveViewDialog dlg(this);
	if (dlg.exec() == QDialog::Accepted)
	{
		QSettings settings;
		settings.beginGroup("views");
		settings.beginGroup(dlg.name());
		settings.setValue("columns", m_logModel->columns());

		QVariant query;
		if (dlg.saveQuery())
			query = m_widget->queryInputCombo->currentText();
		settings.setValue("query", query);

		QList<FilterExpression> filters;
		if (dlg.saveFilters())
		{
			filters = m_filterModel->filters();
			filters.removeAll(FilterExpression());
		}
		saveFiltersArray(settings, filters);

		QVariant start, end;
		if (dlg.saveTimerange())
		{
			auto pair = m_widget->timerangeCombo->currentData(Qt::UserRole).value<QPair<TimeSpec, TimeSpec>>();
			start = pair.first.serialize();
			end = pair.second.serialize();
		}
		settings.setValue("start", start);
		settings.setValue("end", end);
	}
}


void QStuffMainWindow::setInputsEnabled(bool enabled)
{
	m_widget->queryInputCombo->setEnabled(enabled);
	m_widget->timerangeCombo->setEnabled(enabled);
}


void QStuffMainWindow::setupKeysView()
{
	m_keysModel = new QStandardItemModel();
	m_keysProxy = new KeyFilterProxyModel(this);
	m_keysProxy->setSourceModel(m_keysModel);
	m_keysProxy->setSortRole(Qt::UserRole);
	m_widget->keysTree->sortByColumn(0, Qt::AscendingOrder);
	m_widget->keysTree->setModel(m_keysProxy);
	m_widget->keysTree->setItemDelegateForColumn(1, new PercentBarDelegate(500));
	m_keysModel->setHorizontalHeaderLabels({"Key", "Percentage"});

	connect(m_widget->keysTree, &QTreeView::customContextMenuRequested, this, &QStuffMainWindow::showKeysContextMenu);
	connect(m_widget->filterKeysEdit, &QLineEdit::textChanged, m_keysProxy, &QSortFilterProxyModel::setFilterWildcard);

	QAction* toggleKeys = m_widget->keysDock->toggleViewAction();
	toggleKeys->setText("Show &Keys");
	m_widget->menu_Window->addAction(toggleKeys);
}


void QStuffMainWindow::setupFilterView()
{
	m_filterModel = new FilterModel(this);
	m_widget->filterList->setModel(m_filterModel);
	m_widget->filterList->setItemDelegate(new FilterDelegate(m_keysModel, this));

	QSettings settings;
	QList<FilterExpression> filters = loadFiltersArray(settings);
	for (auto filter : filters)
		m_filterModel->addFilter(filter);

	connect(m_widget->filterList->itemDelegate(), &QAbstractItemDelegate::commitData, this, &QStuffMainWindow::search);
	connect(m_filterModel, &FilterModel::checkStateChanged, this, &QStuffMainWindow::search);
	connect(m_widget->addFilterButton, &QToolButton::clicked, [this]{
		int row = m_filterModel->addFilter(FilterExpression());
		m_widget->filterList->edit(m_filterModel->index(row));
	});
	connect(m_widget->enableAllFiltersButton, &QToolButton::clicked, [this]{
		if(m_filterModel->setAllEnabled(true))
			search();
	});
	connect(m_widget->disableAllFiltersButton, &QToolButton::clicked, [this]{
		if (m_filterModel->setAllEnabled(false))
			search();
	});
	connect(m_widget->removeAllFiltersButton, &QToolButton::clicked, [this]{
		if (m_filterModel->removeAllFilters())
			search();
	});

	connect(m_widget->invertFilterButton, &QToolButton::clicked, [this]{
		bool changed = false;
		QModelIndexList selected = m_widget->filterList->selectionModel()->selectedRows();
		if (! selected.isEmpty())
		{
			for (auto index : selected)
				changed |= m_filterModel->invertFilter(index);
		}
		if (changed)
			search();
	});
	connect(m_widget->removeFilterButton, &QToolButton::clicked, [this]{
		QModelIndexList selected = m_widget->filterList->selectionModel()->selectedRows();
		if (! selected.isEmpty())
		{
			QSignalBlocker blocker(m_filterModel);
			for (auto index : selected)
				m_filterModel->removeRow(index.row());
			search();
		}
	});

	connect(m_widget->filterList->selectionModel(), &QItemSelectionModel::selectionChanged, [this]{
		QModelIndexList selected = m_widget->filterList->selectionModel()->selectedRows();
		bool enable_buttons = ! selected.isEmpty();
		m_widget->invertFilterButton->setEnabled(enable_buttons);
		m_widget->removeFilterButton->setEnabled(enable_buttons);
	});

	QAction* toggleFilters = m_widget->filterDock->toggleViewAction();
	toggleFilters->setText("Show &Filters");
	m_widget->menu_Window->addAction(toggleFilters);

}


void QStuffMainWindow::setupChartView()
{
	m_countsChart = new CountsChart(this);
	m_widget->countGraph->setChart(m_countsChart->chart());
	m_widget->countGraph->setRenderHint(QPainter::Antialiasing);
	m_widget->countGraph->setRubberBand(QChartView::HorizontalRubberBand)
;
	connect(m_countsChart->xAxis(), &QDateTimeAxis::rangeChanged, [this](QDateTime min, QDateTime max){
		int index = m_timerangeModel->addChoice(TimeSpec(min), TimeSpec(max));
		m_widget->timerangeCombo->setCurrentIndex(index);
	});

	QAction* toggleChart = m_widget->chartDock->toggleViewAction();
	toggleChart->setText("Show &Chart");
	m_widget->menu_Window->addAction(toggleChart);
}


void QStuffMainWindow::setupReasonableDockWidgetPositions()
{
	addDockWidget(Qt::TopDockWidgetArea, m_widget->chartDock);
	addDockWidget(Qt::LeftDockWidgetArea, m_widget->filterDock);
	addDockWidget(Qt::LeftDockWidgetArea, m_widget->keysDock);
	addDockWidget(Qt::LeftDockWidgetArea, m_widget->logDetailsDock);
	tabifyDockWidget(m_widget->filterDock, m_widget->logDetailsDock);
	tabifyDockWidget(m_widget->filterDock, m_widget->keysDock);
}
