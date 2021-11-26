#ifndef MAINQINDOW_H
#define MAINQINDOW_H

#include <QMainWindow>

namespace Ui {
	class QStuffMainWindow;
}
class QNetworkReply;
class QStandardItemModel;
class QItemSelection;
class LogModel;
class TimerangeModel;
class QSortFilterProxyModel;
class FilterModel;
class QSslConfiguration;
class ChartWidget;

class QStuffMainWindow : public QMainWindow
{
	Q_OBJECT
public:
	enum LastInputFocus { Query, Timerange, Other };
	QStuffMainWindow();

public slots:
	void search();
	void currentLogItemChanged(const QItemSelection& selected, const QItemSelection& /* deselected */);
	void currentTimerangeChanged(int current);
	void toggleKeyColumn(int keyIndex);
	void showKeysContextMenu(const QPoint& point);
	void loadView(const QString& name);
	void saveView();
	void setInputsEnabled(bool enabled);
	void showSettingsDialog();

protected slots:
	void setKeys(const QVariantMap& keys);

signals:
	void startSearch(const QDateTime& start, const QDateTime& end, const QString& query);

protected:
	void closeEvent(QCloseEvent* event) override;
	void saveQueryHistory();
	void loadQueryHistory();
	void loadTimerangeChoices();
	void saveFilters();

private:
	Ui::QStuffMainWindow* m_widget;
	QStandardItemModel* m_keysModel;
	LogModel* m_logModel;
	TimerangeModel* m_timerangeModel;
	LastInputFocus m_lastInputFocus;
	QSortFilterProxyModel* m_keysProxy;
	FilterModel* m_filterModel;
	ChartWidget* m_chartWidget;

	void setupKeysView();
	void setupFilterView();
	void setupChartView();
	void setupReasonableDockWidgetPositions();
};

#endif // MAINQINDOW_H
