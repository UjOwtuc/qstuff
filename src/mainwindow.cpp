#include "mainwindow.h"
#include "percentbardelegate.h"
#include "logmodel.h"
#include "timeinputdialog.h"
#include "timerangemodel.h"
#include "saveviewdialog.h"
#include "keyfilterproxymodel.h"
#include "queryvalidator.h"
#include "settingsdialog.h"
#include "chartwidget.h"
#include "stuffstreamclient.h"
#include "manageviewsdialog.h"
#include "savedviewsmodel.h"
#include "filterlistwidget.h"
#include "syntaxcheckedlineedit.h"

#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardItemModel>
#include <QTimer>
#include <QSettings>
#include <QProgressBar>
#include <QMessageBox>

#include "ui_mainwindow.h"


QStuffMainWindow::QStuffMainWindow()
	: QMainWindow()
{
	m_widget = new Ui::QStuffMainWindow();
	m_widget->setupUi(this);

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

	m_widget->queryInputCombo->setLineEdit(new SyntaxCheckedLineedit(this));
	QueryValidator* validator = new QueryValidator(QueryValidator::Query, this);
	validator->setAcceptEmpty(true);
	m_widget->queryInputCombo->setValidator(validator);
	m_widget->queryInputCombo->lineEdit()->setClearButtonEnabled(true);

	connect(m_widget->queryInputCombo->lineEdit(), &QLineEdit::returnPressed, this, &QStuffMainWindow::search);
	connect(m_widget->timerangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QStuffMainWindow::currentTimerangeChanged);

	connect(m_widget->logsTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, &QStuffMainWindow::currentLogItemChanged);

	QAction* refresh = new QAction(this);
	refresh->setShortcut(Qt::Key_F5);
	connect(refresh, &QAction::triggered, this, &QStuffMainWindow::search);
	addAction(refresh);

	connect(m_widget->action_saveView, &QAction::triggered, this, &QStuffMainWindow::saveView);
	connect(m_widget->action_manageViews, &QAction::triggered, this, &QStuffMainWindow::manageViews);
	connect(m_widget->action_resetView, &QAction::triggered, this, [this]{
		m_logModel->setColumns({"hostname", "programname", "msg"});
		m_widget->logsTable->resizeColumnsToContents();
	});

	QString defaultView = settings.value("default_view", "default").toString();
	settings.beginGroup("views");
	const QStringList& viewNames = settings.childGroups();
	bool defaultViewExists = false;
	for (const QString& name : qAsConst(viewNames))
	{
		QAction* loadViewAction = createLoadViewAction(name);
		if (name == defaultView)
		{
			qDebug() << "default view" << defaultView << "exists, loading it on startup";
			QTimer::singleShot(0, loadViewAction, &QAction::trigger);
			defaultViewExists = true;
		}
	}
	settings.endGroup();

	loadQueryHistory();

	connect(m_widget->action_Settings, &QAction::triggered, this, &QStuffMainWindow::showSettingsDialog);

	StuffstreamClient* client = StuffstreamClient::get();
	client->setBaseUrl(settings.value("stuffstream_url", "http://localhost:8080").toString());
	client->setMaxEvents(settings.value("max_events", 2000).toULongLong());
	client->setTrustedCertificates(settings.value("ca_certificate", "").toString());
	connect(client, &StuffstreamClient::sslErrors, this, [](QNetworkReply* reply, const QList<QSslError>& errors){
		qDebug() << "ssl errors:" << errors;
	});

	connect(client, &StuffstreamClient::receivedFields, this, &QStuffMainWindow::setKeys);
	connect(client, &StuffstreamClient::receivedEvents, this, [this](const QVariantList& events){
		m_widget->detailsTable->clearContents();
		m_widget->detailsTable->setRowCount(0);

		m_logModel->setLogs(events);
		m_widget->logsTable->resizeColumnsToContents();
	});
	connect(client, &StuffstreamClient::receivedMetadata, this, [this](quint64 estimated, quint64 matched){
		m_widget->statusbar->showMessage(
			QString("filter matched %1 of %2 estimated events")
				.arg(matched)
				.arg(estimated));

	});
	connect(client, &StuffstreamClient::requestError, this, [this](const QString& msg){
		QMessageBox::warning(this, "Request failed", msg);
	});

	if (! defaultViewExists)
		QTimer::singleShot(0, this, &QStuffMainWindow::search);
}


void QStuffMainWindow::search()
{
	auto timerange = m_widget->timerangeCombo->currentData(Qt::UserRole).value<QPair<TimeSpec, TimeSpec>>();

	QDateTime start = timerange.first.toDateTime();
	QDateTime end = timerange.second.toDateTime();

	if (start.msecsTo(end) > 0)
	{
		setInputsEnabled(false);
		m_widget->statusbar->clearMessage();

		QString query = m_filterListWidget->combinedFilterString();
		QString input = m_widget->queryInputCombo->currentText();
		if (! input.isEmpty())
		{
			if (query.isEmpty())
				query = input;
			else
				query = QString("(%1) and %2").arg(query, input);
		}

		StuffstreamClient* client = StuffstreamClient::get();
		auto reply = client->fetchEvents(start, end, query);
		emit startSearch(start, end, query);

		connect(reply, &QNetworkReply::finished, this, [this]{
			setInputsEnabled(true);
		});
		QLabel* taskLabel = new QLabel("Waiting for response headers", m_widget->statusbar);
		m_widget->statusbar->addWidget(taskLabel);

		QProgressBar* progress = new QProgressBar(m_widget->statusbar);
		progress->setRange(0, 0);
		m_widget->statusbar->addWidget(progress);

		connect(reply, &QNetworkReply::downloadProgress, this, [progress,taskLabel](quint64 received, quint64 total){
			progress->setMaximum(total);
			progress->setValue(received);
			taskLabel->setText("Receiving events");
		});
		connect(reply, &QNetworkReply::finished, progress, &QProgressBar::deleteLater);
		connect(reply, &QNetworkReply::finished, taskLabel, &QProgressBar::deleteLater);
	}
}


void QStuffMainWindow::setKeys(const QVariantMap& keys)
{
	auto rootItem = m_keysModel->invisibleRootItem();
	QStringList expandedKeys;
	for (int row=0; row<rootItem->rowCount(); ++row)
	{
		if (m_widget->keysTree->isExpanded(m_keysProxy->mapFromSource(m_keysModel->index(row, 0, QModelIndex()))))
			expandedKeys << rootItem->child(row)->data(Qt::DisplayRole).toString();
	}

	rootItem->removeRows(0, rootItem->rowCount());

	auto end = keys.end();
	int row = 0;
	for (auto it=keys.begin(); it!=end; ++it, ++row)
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
	m_filterListWidget->saveFilters();
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


QAction* QStuffMainWindow::createLoadViewAction (const QString& viewName)
{
	const QString data = QString("view %1").arg(viewName);
	QList<QAction*> actions = m_widget->menu_view->actions();
	for (QAction* a : qAsConst(actions))
	{
		if (a->data() == data)
			return a;
	}

	QAction* loadViewAction = new QAction(viewName, this);
	loadViewAction->setData(data);
	connect(loadViewAction, &QAction::triggered, this, [this,viewName]{
		loadView(viewName);
	});
	m_widget->menu_view->addAction(loadViewAction);
	return loadViewAction;
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
			auto valueItem = new QTableWidgetItem(value);
			valueItem->setFlags(valueItem->flags() | Qt::ItemIsEditable);
			m_widget->detailsTable->setItem(row, 1, valueItem);
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
			connect(toggle, &QAction::triggered, this, [this, key]{
				m_logModel->toggleColumn(key);
				m_widget->logsTable->resizeColumnsToContents();
			});
			contextMenu->addAction(toggle);
			QAction* split = new QAction("Split chart by key", contextMenu);
			connect(split, &QAction::triggered, this, [this, key]{
				m_chartWidget->setSplitBy(key);
			});
			contextMenu->addAction(split);
		}
		else
		{
			QString key = m_keysModel->data(index.parent(), Qt::DisplayRole).toString();
			QString value = m_keysModel->data(m_keysModel->index(index.row(), 0, index.parent()), Qt::DisplayRole).toString();

			// TODO: escape quotes in value
			value = QString("\"%1\"").arg(value);
			QAction* filter = new QAction("Filter for value", contextMenu);
			connect(filter, &QAction::triggered, this, [this, key, value]{
				if (m_filterListWidget->addFilter(FilterExpression(key, FilterExpression::Eq, value, false)) >= 0)
					search();
			});
			contextMenu->addAction(filter);

			QAction* filterNot = new QAction("Filter out value", contextMenu);
			connect(filterNot, &QAction::triggered, this, [this, key, value]{
				if (m_filterListWidget->addFilter(FilterExpression(key, FilterExpression::Eq, value, true)) >= 0)
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

	SavedView view(name);
	view.load(settings);

	const QSignalBlocker queryBlocker(m_widget->queryInputCombo);
	const QSignalBlocker timeBlocker(m_widget->timerangeCombo);

	if (view.hasQuery())
		m_widget->queryInputCombo->setCurrentText(view.query());

	if (view.hasColumns())
		m_logModel->setColumns(view.columns());

	if (view.hasTimerange())
	{
		int index = m_timerangeModel->addChoice(view.start(), view.end());
		m_widget->timerangeCombo->setCurrentIndex(index);
	}

	if (view.hasFilters())
		m_filterListWidget->setFilters(view.filters());

	if (view.hasSplitBy())
	{
		m_chartWidget->setSplitBy(view.splitBy());
		m_chartWidget->setLimitBuckets(view.limitBuckets());
	}
	search();
}


void QStuffMainWindow::saveView()
{
	SaveViewDialog dlg(this);

	QString query = m_widget->queryInputCombo->currentText();
	if (query.isEmpty())
		dlg.setSaveQuery(false);
	else
		dlg.setQuery(query);

	dlg.setColumns(m_logModel->columns());
	dlg.setFilters(m_filterListWidget->filters());

	auto pair = m_widget->timerangeCombo->currentData(Qt::UserRole).value<QPair<TimeSpec, TimeSpec>>();
	dlg.setTimerange(pair.first, pair.second);

	dlg.setLimitBuckets(m_chartWidget->limitBuckets());
	QString splitBy = m_chartWidget->splitBy();
	if (splitBy.isEmpty())
		dlg.setSaveSplitBy(false);
	else
		dlg.setSplitBy(splitBy);

	if (dlg.exec() == QDialog::Accepted)
	{
		QSettings settings;
		settings.beginGroup("views");
		settings.beginGroup(dlg.name());
		settings.setValue("columns", m_logModel->columns());

		const SavedView view = dlg.view();
		view.save(settings);
		createLoadViewAction(dlg.name());
	}
}


void QStuffMainWindow::setInputsEnabled(bool enabled)
{
	if (!enabled)
	{
		if (focusWidget() == m_widget->timerangeCombo)
			m_lastInputFocus = Timerange;
		else if (focusWidget() == m_widget->queryInputCombo)
			m_lastInputFocus = Query;
		else
			m_lastInputFocus = Other;
	}

	m_widget->queryInputCombo->setEnabled(enabled);
	m_widget->timerangeCombo->setEnabled(enabled);

	if (enabled)
	{
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
}


void QStuffMainWindow::showSettingsDialog()
{
	StuffstreamClient* client = StuffstreamClient::get();
	SettingsDialog dlg(this);
	dlg.setStuffstreamUrl(client->baseUrl());
	dlg.setMaxEvents(client->maxEvents());
	dlg.setTrustedCerts(client->trustedCertificates());
	dlg.setScaleInterval(m_chartWidget->scaleToInterval());

	if (dlg.exec() == QDialog::Accepted)
	{
		QString searchUrl = dlg.stuffstreamUrl();
		quint64 maxEvents = dlg.maxEvents();
		QString trustedCerts = dlg.trustedCerts();
		quint64 scaleToInterval = dlg.scaleInterval();

		QSettings settings;
		settings.setValue("stuffstream_url", searchUrl);
		settings.setValue("max_events", maxEvents);
		settings.setValue("ca_certificate", trustedCerts);
		settings.setValue("scale_to_interval", scaleToInterval);

		client->setBaseUrl(searchUrl);
		client->setMaxEvents(maxEvents);
		client->setTrustedCertificates(trustedCerts);
		m_chartWidget->setScaleToInterval(scaleToInterval);
		search();
	}
}


void QStuffMainWindow::manageViews()
{
	ManageViewsDialog dlg(this);
	if (dlg.exec() == QDialog::Accepted)
	{
		QSettings settings;
		settings.beginGroup("views");
		SavedView::saveAll(dlg.savedViewsModel()->views(), settings);
	}
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
	m_filterListWidget = new FilterListWidget(m_keysModel, this);
	m_widget->filterDock->setWidget(m_filterListWidget);
	m_filterListWidget->loadFilters();

	connect(m_filterListWidget, &FilterListWidget::filtersChanged, this, &QStuffMainWindow::search);
	QAction* toggleFilters = m_widget->filterDock->toggleViewAction();
	toggleFilters->setText("Show &Filters");
	m_widget->menu_Window->addAction(toggleFilters);
}


void QStuffMainWindow::setupChartView()
{
	m_chartWidget = new ChartWidget(this);
	m_widget->chartDock->setWidget(m_chartWidget);

	QSettings settings;
	m_chartWidget->setScaleToInterval(settings.value("scale_to_interval", 0).toULongLong());

	connect(m_chartWidget, &ChartWidget::timerangeSelected, this, [this](QDateTime min, QDateTime max){
		int index = m_timerangeModel->addChoice(TimeSpec(min), TimeSpec(max));
		m_widget->timerangeCombo->setCurrentIndex(index);
	});

	connect(this, &QStuffMainWindow::startSearch, m_chartWidget, &ChartWidget::fetchCounts);

	connect(m_chartWidget, &ChartWidget::lineClicked, this, [this](const QString& value){
		m_filterListWidget->addFilter(FilterExpression(m_chartWidget->splitBy(), FilterExpression::Eq, QString("\"%1\"").arg(value), false));
		m_chartWidget->setSplitBy("", true);
		search();
	});

	QAction* toggleChart = m_widget->chartDock->toggleViewAction();
	toggleChart->setText("Show &Chart");
	m_widget->menu_Window->addAction(toggleChart);

	m_chartWidget->setSplitChoices(m_keysModel);
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
