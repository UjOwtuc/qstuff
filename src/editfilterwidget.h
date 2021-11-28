#ifndef EDITFILTERWIDGET_H
#define EDITFILTERWIDGET_H

#include "filterexpression.h"

#include <QWidget>

namespace Ui {
	class EditFilterWidget;
}
class QStringListModel;
class QueryValidator;
class QAbstractItemModel;

class EditFilterWidget : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(FilterExpression expression READ expression WRITE setExpression NOTIFY expressionChanged USER true);
	Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged USER true);
public:
	explicit EditFilterWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

	void setCompletionsModel(QAbstractItemModel* model);

	FilterExpression expression() const;
	void setExpression(const FilterExpression& expression);

	QString label() const;
	void setLabel(const QString& label);

public slots:
	void updateValueCompletions();

signals:
	void expressionChanged();
	void labelChanged(QString label);

private:
	Ui::EditFilterWidget* m_widget;
	bool m_labelChanged;
	QAbstractItemModel* m_completionsModel;
	QStringListModel* m_valueCompletions;
	QueryValidator* m_validator;
};

#endif // EDITFILTERWIDGET_H
