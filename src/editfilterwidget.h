#ifndef EDITFILTERWIDGET_H
#define EDITFILTERWIDGET_H

#include <QWidget>
#include "filtermodel.h"

namespace Ui {
	class EditFilterWidget;
}
class QStringListModel;

class EditFilterWidget : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(FilterExpression expression READ expression WRITE setExpression USER true);
	Q_PROPERTY(QString label READ label WRITE setLabel USER true);
public:
	explicit EditFilterWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

	void setCompletionsModel(QAbstractItemModel* model);

	FilterExpression expression() const;
	void setExpression(const FilterExpression& expression);

	QString label() const;
	void setLabel(const QString& label);

public slots:
	void updateValueCompletions();

private:
	Ui::EditFilterWidget* m_widget;
	bool m_labelChanged;
	QAbstractItemModel* m_completionsModel;
	QStringListModel* m_valueCompletions;
};

#endif // EDITFILTERWIDGET_H
