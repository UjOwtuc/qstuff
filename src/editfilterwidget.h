#ifndef EDITFILTERWIDGET_H
#define EDITFILTERWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include "filtermodel.h"

namespace Ui {
	class EditFilterWidget;
}
class QStringListModel;
class QueryValidator;

class SyntaxCheckedLineedit : public QLineEdit
{
	Q_OBJECT
	Q_PROPERTY(QColor okColor MEMBER m_okColor USER true);
	Q_PROPERTY(QColor problemColor MEMBER m_problemColor USER true);
	Q_PROPERTY(QPalette::ColorRole colorRole MEMBER m_colorRole USER true);
public:
	explicit SyntaxCheckedLineedit(QWidget* parent = nullptr);

public slots:
	void checkContent();

private:
	QPalette::ColorRole m_colorRole;
	QColor m_okColor;
	QColor m_problemColor;
};


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
