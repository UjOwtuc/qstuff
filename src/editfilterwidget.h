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
	Q_PROPERTY(QColor okColor READ okColor WRITE setOkColor USER true);
	Q_PROPERTY(QColor problemColor READ problemColor WRITE setProblemColor USER true);
	Q_PROPERTY(QPalette::ColorRole colorRole READ colorRole WRITE setColorRole USER true);
public:
	explicit SyntaxCheckedLineedit(QWidget* parent = nullptr);

	const QColor& okColor() const { return m_okColor; }
	void setOkColor(const QColor& color) { m_okColor = color; }
	const QColor& problemColor() const { return m_problemColor; }
	void setProblemColor(const QColor& color) { m_problemColor = color; }
	QPalette::ColorRole colorRole() const { return m_colorRole; }
	void setColorRole(QPalette::ColorRole group) { m_colorRole = group; }

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
	QueryValidator* m_validator;
};

#endif // EDITFILTERWIDGET_H
