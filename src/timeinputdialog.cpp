#include "timeinputdialog.h"

#include "ui_timeinputdialog.h"
#include "ui_timeinputwidget.h"


TimeInputDialog::TimeInputDialog(QWidget* parent, Qt::WindowFlags f)
	: QDialog(parent, f)
{
	m_dialog = new Ui::TimeInputDialog;
	m_dialog->setupUi(this);

	m_start = new Ui::TimeInputWidget;
	m_start->setupUi(m_dialog->startWidget);

	m_end = new Ui::TimeInputWidget;
	m_end->setupUi(m_dialog->endWidget);

	QDateTime now = QDateTime::currentDateTime();
	m_start->calendarWidget->setCurrentPage(now.date().year(), now.date().month());
	m_start->timeEdit->setTime(QTime(now.time().hour() -1, 0, 0));
	m_end->calendarWidget->setCurrentPage(now.date().year(), now.date().month());
	m_end->timeEdit->setTime(QTime(now.time().hour() +1, 0, 0));

	m_start->numberBox->setValue(1);
	m_end->numberBox->setValue(0);
}


TimeSpec TimeInputDialog::widgetToTimeSpec(Ui::TimeInputWidget* widget)
{
	if (widget->tabWidget->currentIndex() == 0)
	{
		QDateTime selected;
		selected.setDate(widget->calendarWidget->selectedDate());
		selected.setTime(widget->timeEdit->time());
		return TimeSpec(selected);
	}
	TimeSpec::Unit unit;
	switch (widget->comboBox->currentIndex())
	{
		case 0:
			unit = TimeSpec::Minutes;
			break;
		case 1:
			unit = TimeSpec::Hours;
			break;
		case 2:
			unit = TimeSpec::Days;
			break;
		case 3:
			unit = TimeSpec::Weeks;
			break;
		case 4:
			unit = TimeSpec::Months;
			break;
		case 5:
			unit = TimeSpec::Years;
			break;
		default:
			qFatal("Unhandled index in relative start time unit selection");
	}
	return TimeSpec(widget->numberBox->value(), unit);
}


TimeSpec TimeInputDialog::startTime() const
{
	return widgetToTimeSpec(m_start);
}


TimeSpec TimeInputDialog::endTime() const
{
	return widgetToTimeSpec(m_end);
}
