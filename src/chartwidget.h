#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QDateTime>

namespace Ui {
	class ChartWidget;
}
namespace QtCharts {
	class QChart;
	class QDateTimeAxis;
	class QValueAxis;
}

class ChartWidget : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QString splitBy READ splitBy WRITE setSplitBy NOTIFY splitByChanged USER true);
public:
	explicit ChartWidget(QWidget* parent = nullptr);

	QString splitBy() const;

public slots:
	void fetchCounts(const QDateTime& start, const QDateTime& end, const QString& query);
	void update(const QVariantMap& points);
	void fetchIfSplitValueChanged();
	void setSplitBy(const QString& value);

protected slots:
	void sendFetchRequest();

signals:
	void timerangeSelected(QDateTime min, QDateTime max);
	void splitByChanged(QString value);

protected:
	void showEvent(QShowEvent* event) override;

private:
	Ui::ChartWidget* m_widget;
	QDateTime m_lastQueryStart;
	QDateTime m_lastQueryEnd;
	QString m_lastQueryString;
	QString m_currentSplitValue;
	bool m_forceRefresh;

	QtCharts::QChart* m_chart;
	QtCharts::QDateTimeAxis* m_xAxis;
	QtCharts::QValueAxis* m_yAxis;

};

#endif // CHARTWIDGET_H

