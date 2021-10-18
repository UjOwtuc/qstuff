#include "mainwindow.h"
#include "percentbardelegate.h"
#include "logmodel.h"
#include "timeinputdialog.h"
#include "timerangemodel.h"
#include "saveviewdialog.h"
#include "keyfilterproxymodel.h"

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

#include <QtCharts>

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


QStuffMainWindow::QStuffMainWindow()
	: QMainWindow()
{
	m_widget = new Ui::QStuffMainWindow();
	m_widget->setupUi(this);
	m_net_access = new QNetworkAccessManager(this);

	m_top_fields_model = new QStandardItemModel();
	m_keysProxy = new KeyFilterProxyModel(this);
	m_keysProxy->setSourceModel(m_top_fields_model);
	m_keysProxy->setSortRole(Qt::UserRole);
	m_widget->keysTree->sortByColumn(0, Qt::AscendingOrder);
	m_widget->keysTree->setModel(m_keysProxy);
	m_widget->keysTree->setItemDelegateForColumn(1, new PercentBarDelegate(500));
	m_top_fields_model->setHorizontalHeaderLabels({"Key", "Percentage"});
	connect(m_widget->keysTree, &QTreeView::customContextMenuRequested, this, &QStuffMainWindow::showKeysContextMenu);
	connect(m_widget->filterKeysEdit, &QLineEdit::textChanged, m_keysProxy, &QSortFilterProxyModel::setFilterWildcard);

	m_logModel = new LogModel({"hostname", "programname", "msg"});
	m_widget->logsTable->setModel(m_logModel);

	hideDetailsView();

	m_timerangeModel = new TimerangeModel(this);
	m_timerangeModel->addChoice(TimeSpec(15, TimeSpec::Minutes), TimeSpec());
	m_timerangeModel->addChoice(TimeSpec(1, TimeSpec::Hours), TimeSpec());
	m_timerangeModel->addChoice(TimeSpec(4, TimeSpec::Hours), TimeSpec());
	m_timerangeModel->addChoice(TimeSpec(1, TimeSpec::Days), TimeSpec());
	m_timerangeModel->addChoice(TimeSpec(1, TimeSpec::Weeks), TimeSpec());
	m_timerangeModel->addChoice(TimeSpec(1, TimeSpec::Months), TimeSpec());
	m_timerangeModel->addChoice(TimeSpec(1, TimeSpec::Years), TimeSpec());
	m_widget->timerangeCombo->setModel(m_timerangeModel);

	QIcon clearIcon = QIcon::fromTheme("edit-clear", QIcon(QPixmap(clearLineXpm)));
	QAction* clearQuery = m_widget->queryInputCombo->lineEdit()->addAction(clearIcon, QLineEdit::TrailingPosition);;
	connect(clearQuery, &QAction::triggered, [this]{
		m_widget->queryInputCombo->lineEdit()->clear();
		search();
	});

	loadQueryHistory();

	connect(m_widget->queryInputCombo->lineEdit(), &QLineEdit::returnPressed, this, &QStuffMainWindow::search);
	connect(m_net_access, &QNetworkAccessManager::finished, this, &QStuffMainWindow::request_finished);
	connect(m_widget->logsTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, &QStuffMainWindow::currentLogItemChanged);
	connect(m_widget->timerangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QStuffMainWindow::currentTimerangeChanged);
	connect(m_widget->queryInputCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QStuffMainWindow::search);
	connect(m_widget->hideDetailsButton, &QToolButton::clicked, this, &QStuffMainWindow::hideDetailsView);

	QAction* refresh = new QAction(this);
	refresh->setShortcut(Qt::Key_F5);
	connect(refresh, &QAction::triggered, this, &QStuffMainWindow::search);
	addAction(refresh);

	QAction* hideDetails = new QAction(this);
	hideDetails->setShortcut(Qt::Key_Escape);
	connect(hideDetails, &QAction::triggered, this, &QStuffMainWindow::hideDetailsView);
	addAction(hideDetails);

	QTimer::singleShot(0, this, &QStuffMainWindow::search);

	connect(m_widget->action_saveView, &QAction::triggered, this, &QStuffMainWindow::saveView);
	connect(m_widget->action_resetView, &QAction::triggered, [this]{
		m_logModel->setColumns({"hostname", "programname", "msg"});
		m_widget->logsTable->resizeColumnsToContents();
	});
	QSettings settings;
	settings.beginGroup("views");
	for (auto name : settings.childGroups())
	{
		QAction* loadViewAction = new QAction(name, this);
		connect(loadViewAction, &QAction::triggered, [this,name]{
			loadView(name);
		});
		m_widget->menu_View->addAction(loadViewAction);
	}
	settings.endGroup();
}


void QStuffMainWindow::search()
{
	QUrlQuery queryItems;
	auto timerange = m_widget->timerangeCombo->currentData(Qt::UserRole).value<QPair<TimeSpec, TimeSpec>>();

	QDateTime start = timerange.first.toDateTime();
	QDateTime end = timerange.second.toDateTime();

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

		queryItems.addQueryItem("start", start.toUTC().toString(Qt::ISODate));
		queryItems.addQueryItem("end", end.toUTC().toString(Qt::ISODate));
		queryItems.addQueryItem("query", m_widget->queryInputCombo->currentText());
		QUrl url("http://localhost:8080/events");
		url.setQuery(queryItems);
		QNetworkRequest req(url);
		auto reply = m_net_access->get(req);

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


void QStuffMainWindow::request_finished(QNetworkReply* reply)
{
	auto error = reply->error();
	if (error == QNetworkReply::NoError)
	{
		auto queryText = m_widget->queryInputCombo->currentText();
		if (m_widget->queryInputCombo->findText(queryText) < 0)
			m_widget->queryInputCombo->addItem(queryText);

		hideDetailsView();
		QJsonParseError parseError;
		QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
		if (parseError.error == QJsonParseError::NoError)
		{
			QJsonObject obj(doc.object());
			QJsonObject top_fields = obj["fields"].toObject();
			setKeys(top_fields);

			QJsonArray events = obj["events"].toArray();
			m_logModel->setLogs(events.toVariantList());
			m_widget->logsTable->resizeColumnsToContents();

			m_widget->statusbar->showMessage(QString("%1 events in %2").arg(m_logModel->rowCount(QModelIndex())).arg(reply->property("duration string").toString()));
			QLineSeries* countSeries = new QLineSeries();
			QVariantMap counts = obj["counts"].toObject().toVariantMap();
			auto end = counts.constEnd();
			for (auto it=counts.constBegin(); it!=end; ++it)
			{
				QDateTime dt = QDateTime::fromString(it.key(), Qt::ISODate);
				int count = it.value().toInt();
				countSeries->append(dt.toMSecsSinceEpoch(), count);
			}
			QChart* chart = new QChart();
			chart->addSeries(countSeries);
			chart->legend()->hide();

			QDateTimeAxis* xAxis = new QDateTimeAxis();
			xAxis->setTickCount(10);
			xAxis->setFormat("HH:mm");
			xAxis->setTitleText("Time");
			chart->addAxis(xAxis, Qt::AlignBottom);
			countSeries->attachAxis(xAxis);

			QValueAxis* yAxis = new QValueAxis;
			yAxis->setLabelFormat("%i");
			yAxis->setTitleText("Event count");
			chart->addAxis(yAxis, Qt::AlignLeft);
			countSeries->attachAxis(yAxis);
			chart->layout()->setContentsMargins(0, 0, 0, 0);
			chart->setMargins(QMargins());
			m_widget->countGraph->setChart(chart);
			m_widget->countGraph->setRenderHint(QPainter::Antialiasing);
			m_widget->countGraph->setRubberBand(QChartView::HorizontalRubberBand);

			connect(xAxis, &QDateTimeAxis::rangeChanged, [this](QDateTime min, QDateTime max){
				int index = m_timerangeModel->addChoice(TimeSpec(min), TimeSpec(max));
				m_widget->timerangeCombo->setCurrentIndex(index);
			});
		}
		else
			qDebug() << "json parse error:" << parseError.errorString();
	}
	else
	{
		qDebug() << "request error:" << error << reply->readAll();
	}
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
	auto rootItem = m_top_fields_model->invisibleRootItem();
	QStringList expandedKeys;
	for (int row=0; row<rootItem->rowCount(); ++row)
	{
		if (m_widget->keysTree->isExpanded(m_keysProxy->mapFromSource(m_top_fields_model->index(row, 0, QModelIndex()))))
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


void QStuffMainWindow::closeEvent(QCloseEvent* /*event*/)
{
	saveQueryHistory();
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
	QSettings settings;
	int size = settings.beginReadArray("query_history");
	for (int i=0; i<size; ++i)
	{
		settings.setArrayIndex(i);
		m_widget->queryInputCombo->addItem(settings.value("query").toString());
	}
	settings.endArray();
}


void QStuffMainWindow::currentLogItemChanged(const QItemSelection& selected, const QItemSelection& /* deselected */)
{
	m_widget->detailsTable->clearContents();
	if (selected.indexes().isEmpty())
	{
		hideDetailsView();
	}
	else
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
			m_widget->detailsTable->setItem(row, 1, new QTableWidgetItem(it.value().toString()));
		}
		m_widget->detailsTable->resizeColumnsToContents();
		m_widget->detailsTable->resizeRowsToContents();

		auto sizes = m_widget->logDetailsSplitter->sizes();
		if (sizes[1] == 0)
		{
			auto height = m_widget->logDetailsSplitter->height() / 2;
			m_widget->logDetailsSplitter->setSizes({height, height});
			int horizontalPosition = m_widget->logsTable->horizontalScrollBar()->value();
			m_widget->logsTable->scrollTo(current, QAbstractItemView::EnsureVisible);
			m_widget->logsTable->horizontalScrollBar()->setValue(horizontalPosition);
			m_widget->logsTable->selectionModel()->setCurrentIndex(current, QItemSelectionModel::Select | QItemSelectionModel::Rows);
			m_widget->logsTable->selectRow(current.row());
		}
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


void QStuffMainWindow::appendSearch(const QString& append)
{
	qDebug() << "append" << append;
	QString nextQuery(append);
	QString currentQuery = m_widget->queryInputCombo->currentText();
	if (! currentQuery.isEmpty())
		nextQuery = QString("(%1) and %2").arg(currentQuery).arg(append);
	m_widget->queryInputCombo->setCurrentText(nextQuery);
	search();
}


void QStuffMainWindow::toggleKeyColumn(int keyIndex)
{
	QString keyName = m_top_fields_model->item(keyIndex, 0)->text();
	m_logModel->toggleColumn(keyName);
}


void QStuffMainWindow::showKeysContextMenu(const QPoint& point)
{
	QModelIndex index = m_keysProxy->mapToSource(m_widget->keysTree->indexAt(point));
	if (index.isValid())
	{
		QMenu* contextMenu = new QMenu(this);
		if (m_top_fields_model->parent(index) == QModelIndex())
		{
			QString key = m_top_fields_model->data(m_top_fields_model->index(index.row(), 0, index.parent()), Qt::DisplayRole).toString();
			qDebug() << "key" << key;
			QAction* toggle = new QAction("Toggle column in log view", contextMenu);
			connect(toggle, &QAction::triggered, [this,key]{
				m_logModel->toggleColumn(key);
				m_widget->logsTable->resizeColumnsToContents();
			});
			contextMenu->addAction(toggle);
		}
		else
		{
			QString key = m_top_fields_model->data(index.parent(), Qt::DisplayRole).toString();
			QString value = m_top_fields_model->data(m_top_fields_model->index(index.row(), 0, index.parent()), Qt::DisplayRole).toString();
			qDebug() << "value" << value;
			QAction* filter = new QAction("Filter for value", contextMenu);
			connect(filter, &QAction::triggered, [this,key,value]{appendSearch(QString("%1 = \"%2\"").arg(key).arg(value));});
			contextMenu->addAction(filter);
			QAction* filterNot = new QAction("Filter out value", contextMenu);
			connect(filterNot, &QAction::triggered, [this,key,value]{appendSearch(QString("%1 != \"%2\"").arg(key).arg(value));});
			contextMenu->addAction(filterNot);
		}
		contextMenu->exec(m_widget->keysTree->viewport()->mapToGlobal(point));
	}
}


void QStuffMainWindow::hideDetailsView()
{
	m_widget->logDetailsSplitter->setSizes({1, 0});
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
