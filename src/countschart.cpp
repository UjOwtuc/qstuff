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
	m_xAxis->setFormat("HH:mm");
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
		quint64 count = it.value().toULongLong();
		minY = std::min(count, minY);
		maxY = std::max(count, maxY);

		minX = std::min(dt, minX);
		maxX = std::max(dt, maxX);
		m_series->append(dt.toMSecsSinceEpoch(), count);
	}

	QSignalBlocker blocker(m_xAxis);
	m_xAxis->setRange(minX, maxX);
	m_yAxis->setRange(minY, maxY);
}


QtCharts::QChart* CountsChart::chart()
{
	return m_chart;
}


QtCharts::QDateTimeAxis* CountsChart::xAxis()
{
	return m_xAxis;
}
