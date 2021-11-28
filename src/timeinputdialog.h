#ifndef TIMEINPUTDIALOG_H
#define TIMEINPUTDIALOG_H

#include "timespec.h"

#include <QDialog>
#include <QDateTime>

namespace Ui {
	class TimeInputDialog;
	class TimeInputWidget;
}

class TimeInputDialog : public QDialog
{
public:
	explicit TimeInputDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

	TimeSpec startTime() const;
	TimeSpec endTime() const;

private:
	Ui::TimeInputDialog* m_dialog;
	Ui::TimeInputWidget* m_start;
	Ui::TimeInputWidget* m_end;

	static TimeSpec widgetToTimeSpec(Ui::TimeInputWidget* widget);
};

#endif // TIMEINPUTDIALOG_H
