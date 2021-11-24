#include "countschart.h"

#include <QDateTime>
#include <QGraphicsLayout>
#include <QDebug>

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>

using namespace QtCharts;


CountsChart::CountsChart(QObject* parent)
	: QObject(parent)
{
	m_series = new QLineSeries();
	m_chart = new QChart();
	m_chart->addSeries(m_series);
	m_chart->legend()->hide();

	m_xAxis = new QDateTimeAxis();
	m_xAxis->setTickCount(10);
	m_xAxis->setTitleText("Time");
	m_chart->addAxis(m_xAxis, Qt::AlignBottom);
	m_series->attachAxis(m_xAxis);

	m_yAxis = new QValueAxis;
	m_yAxis->setLabelFormat("%i");
	m_yAxis->setTitleText("Event count");
	m_chart->addAxis(m_yAxis, Qt::AlignLeft);
	m_series->attachAxis(m_yAxis);
	m_chart->layout()->setContentsMargins(0, 0, 0, 0);
	m_chart->setMargins(QMargins());
}


void CountsChart::plotCounts(const QVariantMap& counts)
{
	m_series->clear();
	QDateTime minX, maxX;
	minX = QDateTime::currentDateTime().addYears(10000000);  // couldn't find a sane maximum value for QDateTime
	quint64 minY=std::numeric_limits<quint64>::max(), maxY=0;

	auto end = counts.constEnd();
	for (auto it=counts.constBegin(); it!=end; ++it)
	{
		QDateTime dt = QDateTime::fromString(it.key(), Qt::ISODate);
		if (dt.isValid())
		{
			quint64 count = it.value().toULongLong();
			minY = std::min(count, minY);
			maxY = std::max(count, maxY);

			minX = std::min(dt, minX);
			maxX = std::max(dt, maxX);
			m_series->append(dt.toMSecsSinceEpoch(), count);
		}
		else
			qDebug() << "invalid date in counts:" << it.key();
	}

	qint64 duration = minX.secsTo(maxX);
	switch (duration)
	{
		case 0 ... 1800:
			m_xAxis->setFormat("HH:mm:ss.zzz");
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
	m_yAxis->setRange(minY, maxY);
}


void CountsChart::setInterval(int seconds)
{
	QString label;
	switch (seconds)
	{
		case 1:
			label = "second";
			break;
		case 2 ... 59:
			label = QString("%1 seconds").arg(seconds);
			break;
		case 60:
			label = "minute";
			break;
		case 61 ... 3599:
			if (seconds % 60)
				label = QString("%1:%2 minutes").arg(int(seconds / 60)).arg(seconds % 60, 2, QChar('0'));
			else
				label = QString("%1 minutes").arg(int(seconds / 60));
			break;
		case 3600:
			label = "hour";
			break;
		case 3601 ... 24 * 3600 -1:
		{
			int minutes = seconds / 60;
			if (minutes % 60)
				label = QString("%1:%2 hours").arg(int(minutes / 60)).arg(minutes % 60, 2, QChar('0'));
			else
				label = QString("%1 hours").arg(int(minutes / 60));
			break;
		}
		case 24 * 3600:
			label = "day";
			break;
	}
	m_yAxis->setTitleText(QString("Events per %1").arg(label));
}


QtCharts::QChart* CountsChart::chart()
{
	return m_chart;
}


QtCharts::QDateTimeAxis* CountsChart::xAxis()
{
	return m_xAxis;
}
