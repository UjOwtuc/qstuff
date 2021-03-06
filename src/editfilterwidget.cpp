#include "editfilterwidget.h"
#include "queryvalidator.h"
#include "syntaxcheckedlineedit.h"

#include <QCompleter>
#include <QStringListModel>
#include <QDebug>
#include <QColor>

#include "ui_editfilterwidget.h"


EditFilterWidget::EditFilterWidget(QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f)
{
	m_widget = new Ui::EditFilterWidget;
	m_widget->setupUi(this);

	m_validator = new QueryValidator(QueryValidator::Scalar, this);
	m_valueCompletions = nullptr;

	const QMap<FilterExpression::Op, QString>& ops = FilterExpression::ops();
	auto end = ops.constEnd();
	for (auto it=ops.constBegin(); it != end; ++it)
		m_widget->opCombo->addItem(it.value(), it.key());

	m_widget->idCombo->setLineEdit(new SyntaxCheckedLineedit(this));
	m_widget->idCombo->setValidator(new QueryValidator(QueryValidator::Field, this));
	m_widget->valueCombo->setLineEdit(new SyntaxCheckedLineedit(this));
	m_widget->valueCombo->setValidator(m_validator);
	connect(m_widget->opCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
		if (index < 0)
			m_widget->opCombo->setCurrentIndex(0);
		else
		{
			auto op = static_cast<FilterExpression::Op>(index);
			switch (op)
			{
				case FilterExpression::Eq:
				case FilterExpression::Ge:
				case FilterExpression::Gt:
				case FilterExpression::Le:
				case FilterExpression::Lt:
				case FilterExpression::Like:
					m_validator->setRule(QueryValidator::Scalar);
					break;
				case FilterExpression::In:
					m_validator->setRule(QueryValidator::List);
					break;
			}
			qobject_cast<SyntaxCheckedLineedit*>(m_widget->valueCombo->lineEdit())->checkContent();
		}
		emit expressionChanged();
	});
	setFocusProxy(m_widget->idCombo);

	connect(m_widget->idCombo, &QComboBox::currentTextChanged, this, &EditFilterWidget::expressionChanged);
	connect(m_widget->valueCombo, &QComboBox::currentTextChanged, this, &EditFilterWidget::expressionChanged);
	connect(m_widget->customLabelEdit, &QLineEdit::textChanged, this, &EditFilterWidget::labelChanged);
}


void EditFilterWidget::setCompletionsModel(QAbstractItemModel* model)
{
	m_completionsModel = model;
	m_widget->idCombo->setCompleter(new QCompleter(m_completionsModel, this));

	if (! m_valueCompletions)
		m_valueCompletions = new QStringListModel(this);

	connect(m_widget->idCombo, &QComboBox::currentTextChanged, this, &EditFilterWidget::updateValueCompletions);
	m_widget->valueCombo->setCompleter(new QCompleter(m_valueCompletions, this));
}


FilterExpression EditFilterWidget::expression() const
{
	FilterExpression expr(
		m_widget->idCombo->currentText(),
		static_cast<FilterExpression::Op>(m_widget->opCombo->itemData(m_widget->opCombo->currentIndex(), Qt::UserRole).toInt()),
		m_widget->valueCombo->currentText(),
		m_widget->invertFilterCheckbox->isChecked()
	);
	expr.setEnabled(m_widget->enabledCheckBox->isChecked());
	return expr;
}


void EditFilterWidget::setExpression(const FilterExpression& expression)
{
	m_widget->idCombo->setCurrentText(expression.id());
	m_widget->opCombo->setCurrentIndex(m_widget->opCombo->findData(static_cast<int>(expression.op())));
	m_widget->valueCombo->setCurrentText(expression.value());
	m_widget->invertFilterCheckbox->setChecked(expression.isInverted());
	m_widget->enabledCheckBox->setChecked(expression.enabled());

	if (expression.hasCustomLabel())
		setLabel(expression.label());
	emit expressionChanged();
}


QString EditFilterWidget::label() const
{
	return m_widget->customLabelEdit->text();
}


void EditFilterWidget::setLabel(const QString& label)
{
	if (label != m_widget->customLabelEdit->text())
	{
		m_widget->customLabelEdit->setText(label);
		m_labelChanged = true;
	}
	emit labelChanged(label);
}


void EditFilterWidget::updateValueCompletions()
{
	QString key = m_widget->idCombo->currentText();
	int rowCount = m_completionsModel->rowCount();
	QModelIndex parentIndex;
	for (int row=0; row < rowCount; ++row)
	{
		QModelIndex current = m_completionsModel->index(row, 0);
		if (m_completionsModel->data(m_completionsModel->index(row, 0)) == key)
		{
			parentIndex = current;
			break;
		}
	}

	QStringList completions;
	if (parentIndex.isValid())
	{
		rowCount = m_completionsModel->rowCount(parentIndex);
		for (int row=0; row < rowCount; ++row)
			completions << m_completionsModel->data(m_completionsModel->index(row, 0, parentIndex)).toString();
	}
	m_valueCompletions->setStringList(completions);
}
