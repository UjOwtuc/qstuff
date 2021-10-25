#ifndef COUNTSGRAPH_H
#define COUNTSGRAPH_H

#include <QObject>
namespace QtCharts {
	class QLineSeries;
	class QChart;
	class QDateTimeAxis;
	class QValueAxis;
}


class CountsChart : public QObject
{
public:
	explicit CountsChart(QObject* parent = nullptr);

	void plotCounts(const QVariantMap& counts);
	void setInterval(int seconds);

	QtCharts::QChart* chart();
	QtCharts::QDateTimeAxis* xAxis();

private:
	QtCharts::QLineSeries* m_series;
	QtCharts::QChart* m_chart;
	QtCharts::QDateTimeAxis* m_xAxis;
	QtCharts::QValueAxis* m_yAxis;
};

#endif // COUNTSGRAPH_H
