#include "chartwidget.h"
#include "editfilterwidget.h"
#include "queryvalidator.h"
#include "stuffstreamclient.h"

#include <QDebug>

#include "ui_chartwidget.h"


QString prettyInterval(quint64 seconds)
{
	QString pretty;
	switch (seconds)
	{
		case 1:
			pretty = "second";
			break;
		case 2 ... 59:
			pretty = QString("%1 seconds").arg(seconds);
			break;
		case 60:
			pretty = "minute";
			break;
		case 61 ... 3599:
			if (seconds % 60)
				pretty = QString("%1:%2 minutes").arg(int(seconds / 60)).arg(seconds % 60, 2, 10, QChar('0'));
			else
				pretty = QString("%1 minutes").arg(int(seconds / 60));
			break;
		case 3600:
			pretty = "hour";
			break;
		case 3601 ... 24 * 3600 -1:
		{
			int minutes = seconds / 60;
			if (minutes % 60)
				pretty = QString("%1:%2 hours").arg(int(minutes / 60)).arg(minutes % 60, 2, 10, QLatin1Char('0'));
			else
				pretty = QString("%1 hours").arg(int(minutes / 60));
			break;
		}
		case 24 * 3600:
			pretty = "day";
			break;
	}
	return pretty;
}


ChartWidget::ChartWidget(QWidget* parent)
	: QWidget(parent)
{
	m_scaleToInterval = 0;

	m_widget = new Ui::ChartWidget();
	m_widget->setupUi(this);

	m_widget->splitCombo->setLineEdit(new SyntaxCheckedLineedit(this));
	QueryValidator* validator = new QueryValidator(QueryValidator::Field, this);
	validator->setAcceptEmpty(true);
	m_widget->splitCombo->setValidator(validator);
	m_widget->splitCombo->lineEdit()->setClearButtonEnabled(true);

	m_xAxis = new QDateTimeAxis();
	m_xAxis->setTickCount(10);
	m_xAxis->setTitleText("Time");

	m_yAxis = new QValueAxis;
	m_yAxis->setLabelFormat("%i");
	m_yAxis->setTitleText("Event count");

	m_chart = new QChart();
	m_chart->setLocalizeNumbers(true);
	m_chart->addAxis(m_xAxis, Qt::AlignBottom);
	m_chart->addAxis(m_yAxis, Qt::AlignLeft);
	m_chart->layout()->setContentsMargins(0, 0, 0, 0);
	m_chart->setMargins(QMargins());

	m_widget->chartView->setChart(m_chart);
	m_widget->chartView->setRenderHint(QPainter::Antialiasing);
	m_widget->chartView->setRubberBand(QChartView::HorizontalRubberBand);

	connect(m_xAxis, &QDateTimeAxis::rangeChanged, this, &ChartWidget::timerangeSelected);
	connect(StuffstreamClient::get(), &StuffstreamClient::receivedCounts, this, &ChartWidget::update);
	connect(m_widget->splitCombo->lineEdit(), &QLineEdit::editingFinished, this, &ChartWidget::fetchIfSplitValueChanged);
	connect(m_widget->splitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ChartWidget::fetchIfSplitValueChanged);
	connect(m_widget->limitBucketsSpinbox, &QSpinBox::editingFinished, this, &ChartWidget::fetchIfSplitValueChanged);
}


void ChartWidget::setSplitChoices(QAbstractItemModel* model)
{
	QCompleter* completer = new QCompleter(this);
	completer->setModel(model);
	m_widget->splitCombo->setCompleter(completer);
}


qreal ChartWidget::scaleValueToInterval(quint64 value)
{
	if (m_scaleToInterval == 0)
		return value;

	return m_scaleToInterval * value / static_cast<qreal>(m_currentInterval);
}


void ChartWidget::fetchCounts(const QDateTime& start, const QDateTime& end, const QString& query)
{
	m_lastQueryStart = start;
	m_lastQueryEnd = end;
	m_lastQueryString = query;
	m_forceRefresh = true;
	if (isVisible())
		sendFetchRequest();
}


void ChartWidget::generateSeries(const QString& name, const QVariantMap& map, quint64& minValue, quint64& maxValue, const std::function<quint64(const QVariantMap::const_iterator&)>& valueGetter)
{
	QLineSeries* series = new QLineSeries();
	series->setPointsVisible(true);

	auto end = map.constEnd();
	for (auto it=map.constBegin(); it!=end; ++it)
	{
		QDateTime dt = QDateTime::fromString(it.key(), Qt::ISODate);
		if (dt.isValid())
		{
			quint64 value = valueGetter(it);
			minValue = std::min(value, minValue);
			maxValue = std::max(value, maxValue);
			series->append(dt.toMSecsSinceEpoch(), scaleValueToInterval(value));
		}
		else
			qDebug() << "invalid date in points:" << it.key();
	}

	QSignalBlocker blocker(m_xAxis);
	m_chart->addSeries(series);
	series->attachAxis(m_xAxis);
	series->attachAxis(m_yAxis);

	if (! name.isEmpty())
	{
		connect(series, &QXYSeries::clicked, this, [this, name]{
			emit lineClicked(name);
		});
		series->setName(name);
	}
}

void ChartWidget::update(const QVariantMap& points)
{
	m_chart->removeAllSeries();

	if (points.isEmpty())
		return;

	const QDateTime minX = QDateTime::fromString(points.firstKey(), Qt::ISODate);
	const QDateTime maxX = QDateTime::fromString(points.lastKey(), Qt::ISODate);
	quint64 minY=std::numeric_limits<quint64>::max(), maxY=0;

	auto it = points.constKeyValueBegin();
	++it;
	const QDateTime secondPoint = QDateTime::fromString(it->first, Qt::ISODate);
	m_currentInterval = minX.secsTo(secondPoint);
	m_yAxis->setTitleText(QString("Events per %1").arg(prettyInterval(m_scaleToInterval == 0 ? m_currentInterval : m_scaleToInterval)));


	QVariant firstValue = points.first();
	if (firstValue.canConvert(QMetaType::ULongLong))
	{
		m_chart->legend()->hide();
		generateSeries("", points, minY, maxY, [](const QVariantMap::const_iterator& it) {
			return it->toULongLong();
		});
	}
	else
	{
		m_chart->legend()->show();
		QStringList names = firstValue.toMap().keys();
		for (const QString& name : qAsConst(names))
		{
			generateSeries(name, points, minY, maxY, [name](const QVariantMap::const_iterator& it) {
				return it->toMap()[name].toULongLong();
			});
		}
	}

	qint64 duration = minX.secsTo(maxX);
	switch (duration)
	{
		case 0 ... 1800:
			m_xAxis->setFormat("HH:mm:ss");
			break;
		case 1801 ... 36 * 3600:
			m_xAxis->setFormat("HH:mm");
			break;
		case 36 * 3600 + 1 ... 90 * 24 * 3600:
			m_xAxis->setFormat("dd.MM HH:mm");
			break;
		default:
			m_xAxis->setFormat("dd.MM.yyyy HH:mm");
	}

	QSignalBlocker blocker(m_xAxis);
	m_xAxis->setRange(minX, maxX);
	m_yAxis->setRange(scaleValueToInterval(minY), scaleValueToInterval(maxY));
	m_yAxis->applyNiceNumbers();

	if ((scaleValueToInterval(maxY) - scaleValueToInterval(minY)) < 5)
		m_yAxis->setLabelFormat("%.2f");
	else
		m_yAxis->setLabelFormat("%.0f");
}


void ChartWidget::fetchIfSplitValueChanged()
{
	bool changed = false;
	if (m_widget->splitCombo->currentText() != m_currentSplitValue)
	{
		m_currentSplitValue = m_widget->splitCombo->currentText();
		emit splitByChanged(m_currentSplitValue);
		changed = true;
	}
	if (m_widget->limitBucketsSpinbox->value() != m_currentBucketsLimit)
	{
		m_currentBucketsLimit = m_widget->limitBucketsSpinbox->value();
		emit limitBucketsChanged(m_currentBucketsLimit);
		changed = true;
	}

	if (changed)
		sendFetchRequest();
}


QString ChartWidget::splitBy() const
{
	return m_widget->splitCombo->currentText();
}


void ChartWidget::setSplitBy(const QString& value, bool preventRefresh)
{
	m_widget->splitCombo->setCurrentText(value);
	if (! preventRefresh)
		fetchIfSplitValueChanged();
}


quint32 ChartWidget::limitBuckets() const
{
	return m_widget->limitBucketsSpinbox->value();
}


void ChartWidget::setLimitBuckets(quint32 value, bool preventRefresh)
{
	m_widget->limitBucketsSpinbox->setValue(value);
	if (! preventRefresh)
		fetchIfSplitValueChanged();
}


void ChartWidget::setScaleToInterval(quint64 value)
{
	if (value != m_scaleToInterval)
	{
		m_scaleToInterval = value;
		sendFetchRequest();
		emit scaleToIntervalChanged(value);
	}
}


void ChartWidget::sendFetchRequest()
{
	if (m_lastQueryStart.isValid() && m_lastQueryEnd.isValid())
	{
		StuffstreamClient* client = StuffstreamClient::get();
		client->fetchCounts(m_lastQueryStart, m_lastQueryEnd, m_lastQueryString, m_widget->splitCombo->currentText(), m_widget->limitBucketsSpinbox->value());
		m_forceRefresh = false;
	}
}


void ChartWidget::showEvent(QShowEvent* event)
{
	if (m_forceRefresh)
		sendFetchRequest();

	QWidget::showEvent(event);
}
