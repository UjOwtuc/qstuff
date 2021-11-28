#ifndef SYNTAXCHECKEDLINEEDIT_H
#define SYNTAXCHECKEDLINEEDIT_H

#include <QLineEdit>

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

#endif // SYNTAXCHECKEDLINEEDIT_H
