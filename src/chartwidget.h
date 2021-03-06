#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QDateTime>
#include <QVariant>

namespace Ui {
	class ChartWidget;
}
namespace QtCharts {
	class QChart;
	class QDateTimeAxis;
	class QValueAxis;
}
class QAbstractItemModel;

class ChartWidget : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QString splitBy READ splitBy WRITE setSplitBy NOTIFY splitByChanged USER true);
	Q_PROPERTY(quint32 limitBuckets READ limitBuckets WRITE setLimitBuckets NOTIFY limitBucketsChanged);
	Q_PROPERTY(quint64 scaleToInterval READ scaleToInterval WRITE setScaleToInterval NOTIFY scaleToIntervalChanged);
public:
	explicit ChartWidget(QWidget* parent = nullptr);

	QString splitBy() const;
	quint32 limitBuckets() const;
	quint64 scaleToInterval() const { return m_scaleToInterval; }

	void setSplitChoices(QAbstractItemModel* model);
	qreal scaleValueToInterval(quint64 value);

public slots:
	void fetchCounts(const QDateTime& start, const QDateTime& end, const QString& query);
	void update(const QVariantMap& points);
	void fetchIfSplitValueChanged();
	void setSplitBy(const QString& value, bool preventRefresh=false);
	void setLimitBuckets(quint32 value, bool preventRefresh=false);
	void setScaleToInterval(quint64 value);

protected slots:
	void sendFetchRequest();

signals:
	void timerangeSelected(QDateTime min, QDateTime max);
	void splitByChanged(QString value);
	void limitBucketsChanged(quint32 value);
	void lineClicked(const QString& name);
	void scaleToIntervalChanged(quint64 value);
	void metricChanged(QString metric);
	void aggregateChanged(QString aggregate);

protected:
	void showEvent(QShowEvent* event) override;
	void generateSeries(const QString& name, const QVariantMap& map, quint64& minValue, quint64& maxValue, const std::function<QVariant(const QVariantMap::const_iterator&)>& valueGetter);

private:
	Ui::ChartWidget* m_widget;
	QDateTime m_lastQueryStart;
	QDateTime m_lastQueryEnd;
	QString m_lastQueryString;
	QString m_currentSplitValue;
	quint32 m_currentBucketsLimit;
	QString m_currentMetric;
	QString m_currentAggregate;
	bool m_forceRefresh;
	quint64 m_scaleToInterval;
	quint64 m_currentInterval;

	QtCharts::QChart* m_chart;
	QtCharts::QDateTimeAxis* m_xAxis;
	QtCharts::QValueAxis* m_yAxis;

};

#endif // CHARTWIDGET_H

