#include "settingsdialog.h"

#include "ui_settingsdialog.h"


SettingsDialog::SettingsDialog(QWidget* parent, Qt::WindowFlags f)
	: QDialog(parent, f)
{
	m_widget = new Ui::SettingsDialog;
	m_widget->setupUi(this);
}


QString SettingsDialog::stuffstreamUrl() const
{
	return m_widget->stuffstreamUrlEdit->text();
}


void SettingsDialog::setStuffstreamUrl(const QString& value)
{
	m_widget->stuffstreamUrlEdit->setText(value);
}


quint16 SettingsDialog::maxEvents() const
{
	return m_widget->maxEventsSpinbox->value();
}


void SettingsDialog::setMaxEvents(quint64 value)
{
	m_widget->maxEventsSpinbox->setValue(value);
}
