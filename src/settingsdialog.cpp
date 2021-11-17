#include "settingsdialog.h"

#include <QCompleter>
#include <QFileSystemModel>
#include <QDebug>
#include <QTimer>

#include "ui_settingsdialog.h"


class FileSystemModel : public QFileSystemModel
{
public:
	explicit FileSystemModel(QObject *parent = nullptr)
		: QFileSystemModel(parent)
	{
		setRootPath(QString());
	}
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
	{
		if (role == Qt::DisplayRole && index.column() == 0)
		{
			QString path = QDir::toNativeSeparators(filePath(index));
			return path;
		}

		return QFileSystemModel::data(index, role);
	}
};


SettingsDialog::SettingsDialog(QWidget* parent, Qt::WindowFlags f)
	: QDialog(parent, f)
{
	m_widget = new Ui::SettingsDialog;
	m_widget->setupUi(this);

	QCompleter* completer = new QCompleter(this);
	completer->setModel(new FileSystemModel(completer));
	m_widget->trustedCertsEdit->setCompleter(completer);

	connect(m_widget->trustedCertsEdit, &QLineEdit::textChanged, [this, completer](const QString& text){
		QFileInfo file(text);
		if (file.exists() && file.isDir())
		{
			if (! text.endsWith(QDir::separator()))
			{
				QSignalBlocker blocker(m_widget->trustedCertsEdit);
				m_widget->trustedCertsEdit->setText(text + QDir::separator());
			}
			QMetaObject::invokeMethod(completer, "complete", Qt::QueuedConnection);
		}
	});
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


QString SettingsDialog::trustedCerts() const
{
	return m_widget->trustedCertsEdit->text();
}


void SettingsDialog::setTrustedCerts(const QString& value)
{
	m_widget->trustedCertsEdit->setText(value);
}
