#include "syntaxcheckedlineedit.h"


SyntaxCheckedLineedit::SyntaxCheckedLineedit(QWidget* parent)
	: QLineEdit(parent),
	m_colorRole(QPalette::Text),
	m_okColor(Qt::black),
	m_problemColor(Qt::red)
{
	connect(this, &QLineEdit::textChanged, this, &SyntaxCheckedLineedit::checkContent);
}


void SyntaxCheckedLineedit::checkContent()
{
	auto pal = palette();
	bool ok = hasAcceptableInput();
	if ((ok && pal.color(m_colorRole) != m_okColor) || (!ok && pal.color(m_colorRole) != m_problemColor))
	{
		pal.setColor(m_colorRole, ok ? m_okColor : m_problemColor);
		setPalette(pal);
	}
}
