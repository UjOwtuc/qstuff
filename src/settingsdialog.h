#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
	class SettingsDialog;
}

class SettingsDialog : public QDialog
{
	Q_PROPERTY(QString stuffstream_url READ stuffstreamUrl WRITE setStuffstreamUrl USER true);
	Q_PROPERTY(quint64 max_events READ maxEvents WRITE setMaxEvents USER true);

public:
	explicit SettingsDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

	QString stuffstreamUrl() const;
	void setStuffstreamUrl(const QString& value);

	quint16 maxEvents() const;
	void setMaxEvents(quint64 value);

private:
	Ui::SettingsDialog* m_widget;
};

#endif // SETTINGSDIALOG_H
