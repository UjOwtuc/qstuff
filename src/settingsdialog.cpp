#include "settingsdialog.h"

#include <QCompleter>
#include <QFileSystemModel>
#include <QDebug>
#include <QTimer>
#include <QFileDialog>
#include <QSpinBox>

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

	m_widget->scaleIntervalCombo->addItem("Don't scale", 0);
	m_widget->scaleIntervalCombo->addItem("1 second", 1);
	m_widget->scaleIntervalCombo->addItem("1 minute", 60);
	m_widget->scaleIntervalCombo->addItem("1 hour", 3600);

	connect(m_widget->trustedCertsEdit, &QLineEdit::textChanged, this, [this, completer](const QString& text){
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
		emit trustedCertsChanged(text);
	});

	connect(m_widget->trustedCertsBrowseButton, &QToolButton::clicked, this, [this]{
		QString filename = QFileDialog::getOpenFileName(
			this,
			"Trusted Certificate Bundle",
			m_widget->trustedCertsEdit->text(),
			"PEM Certificates (*.crt *.pem *.cert)"
		);
		if (! filename.isEmpty())
			m_widget->trustedCertsEdit->setText(filename);
	});

	connect(m_widget->maxEventsSpinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDialog::maxEventsChanged);
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


quint64 SettingsDialog::scaleInterval() const
{
	return m_widget->scaleIntervalCombo->currentData().toULongLong();
}


void SettingsDialog::setScaleInterval(quint64 value)
{
	int index = m_widget->scaleIntervalCombo->findData(value);
	if (! index)
	{
		m_widget->scaleIntervalCombo->addItem(QString("%1 seconds").arg(value), value);
		index = m_widget->scaleIntervalCombo->findData(value);
	}
	m_widget->scaleIntervalCombo->setCurrentIndex(index);
}
