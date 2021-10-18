#ifndef MAINQINDOW_H
#define MAINQINDOW_H

#include <QMainWindow>

namespace Ui {
	class QStuffMainWindow;
}
class QNetworkAccessManager;
class QNetworkReply;
class QStandardItemModel;
class QItemSelection;
class LogModel;
class TimerangeModel;

class QStuffMainWindow : public QMainWindow
{
public:
	enum LastInputFocus { Query, Timerange, Other };
	QStuffMainWindow();

public slots:
	void search();
	void currentLogItemChanged(const QItemSelection& selected, const QItemSelection& /* deselected */);
	void currentTimerangeChanged(int current);
	void appendSearch(const QString& append);
	void toggleKeyColumn(int keyIndex);
	void showKeysContextMenu(const QPoint& point);
	void hideDetailsView();
	void loadView(const QString& name);
	void saveView();
	void setInputsEnabled(bool enabled);

protected slots:
	void request_finished(QNetworkReply* reply);

protected:
	void setKeys(const QJsonObject& keys);
	void closeEvent(QCloseEvent* /*event*/) override;
	void saveQueryHistory();
	void loadQueryHistory();

private:
	Ui::QStuffMainWindow* m_widget;
	QNetworkAccessManager* m_net_access;
	QStandardItemModel* m_top_fields_model;
	LogModel* m_logModel;
	TimerangeModel* m_timerangeModel;
	LastInputFocus m_lastInputFocus;
};

#endif // MAINQINDOW_H
