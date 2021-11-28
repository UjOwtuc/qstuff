#ifndef FILTERLISTWIDGET_H
#define FILTERLISTWIDGET_H

#include <QWidget>

namespace Ui {
	class FilterListWidget;
}
class FilterModel;
class FilterExpression;
class QAbstractItemModel;

class FilterListWidget : public QWidget
{
	Q_OBJECT
public:
	explicit FilterListWidget(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
	explicit FilterListWidget(QAbstractItemModel* editorCompletionModel, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

	const QList<FilterExpression>& filters() const;
	QString combinedFilterString() const;
	void loadFilters();
	void saveFilters() const;
	int addFilter(const FilterExpression& expr);
	void setFilters(const QList<FilterExpression>& filters);
	void setFiltersEditable(bool set);

public slots:
	void invertSelected();
	void removeSelected();
	void removeAll();
	bool checkFiltersChanged();

signals:
	void filtersChanged();

private:
	Ui::FilterListWidget* m_widget;
	FilterModel* m_filterModel;

	QString m_lastFilterState;

	void setUp();
};

#endif // FILTERLISTWIDGET_H
