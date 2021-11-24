#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
	class SettingsDialog;
}

class SettingsDialog : public QDialog
{
	Q_OBJECT
	Q_PROPERTY(QString stuffstream_url READ stuffstreamUrl WRITE setStuffstreamUrl NOTIFY urlChanged USER true);
	Q_PROPERTY(quint64 max_events READ maxEvents WRITE setMaxEvents NOTIFY maxEventsChanged USER true);
	Q_PROPERTY(QString trusted_certs READ trustedCerts WRITE setTrustedCerts NOTIFY trustedCertsChanged USER true);

public:
	explicit SettingsDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

	QString stuffstreamUrl() const;
	void setStuffstreamUrl(const QString& value);

	quint16 maxEvents() const;
	void setMaxEvents(quint64 value);

	QString trustedCerts() const;
	void setTrustedCerts(const QString& value);

signals:
	void urlChanged(QString);
	void maxEventsChanged(quint64);
	void trustedCertsChanged(QString);

private:
	Ui::SettingsDialog* m_widget;
};

#endif // SETTINGSDIALOG_H
