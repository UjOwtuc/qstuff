#include "editfilterwidget.h"
#include "filtermodel.h"

#include <QCompleter>
#include <QStringListModel>
#include <QDebug>

#include "ui_editfilterwidget.h"


class ValueValidator : public QValidator
{
public:
	explicit ValueValidator(QObject* parent = nullptr) : QValidator(parent) {}

	void fixup(QString& input) const override
	{
		bool ok;
		input.toDouble(&ok);
		if (! ok && input.length() > 0 && input.front() != '"')
			input.prepend('"');
		if (! ok && input.length() > 0 && input.back() != '"')
			input.append('"');
	}

	QValidator::State validate(QString& input, int& /*pos*/) const override
	{
		bool ok;
		input.toDouble(&ok);
		if (ok)
			return Acceptable;

		if (input.length() >= 2 && input.front() == '"' && input.back() == '"')
			return Acceptable;

		return Intermediate;
	}
};


EditFilterWidget::EditFilterWidget(QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f)
{
	m_widget = new Ui::EditFilterWidget;
	m_widget->setupUi(this);

	m_valueCompletions = nullptr;

	const QMap<FilterExpression::Op, QString>& ops = FilterExpression::ops();
	auto end = ops.constEnd();
	for (auto it=ops.constBegin(); it != end; ++it)
		m_widget->opCombo->addItem(it.value(), it.key());

	m_widget->valueCombo->setValidator(new ValueValidator(this));
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
		m_widget->valueCombo->currentText()
	);
	expr.setEnabled(m_widget->enabledCheckBox->isChecked());
	return expr;
}


void EditFilterWidget::setExpression(const FilterExpression& expression)
{
	m_widget->idCombo->setCurrentText(expression.id());
	m_widget->opCombo->setCurrentIndex(m_widget->opCombo->findData(static_cast<int>(expression.op())));
	m_widget->valueCombo->setCurrentText(expression.value());
	m_widget->enabledCheckBox->setChecked(expression.enabled());
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
		int rowCount = m_completionsModel->rowCount(parentIndex);
		for (int row=0; row < rowCount; ++row)
			completions << m_completionsModel->data(m_completionsModel->index(row, 0, parentIndex)).toString();
	}
	m_valueCompletions->setStringList(completions);
}
