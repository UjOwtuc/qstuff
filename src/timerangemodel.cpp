#include "timerangemodel.h"
#include "timeinputdialog.h"

#include <QApplication>
#include <QSettings>
#include <QDebug>

TimerangeModel::TimerangeModel(QObject* parent)
	: QAbstractListModel(parent)
{
	QSettings settings;
	int size = settings.beginReadArray("timerange_choices");
	for (int i=0; i<size; ++i)
	{
		settings.setArrayIndex(i);
		TimeSpec start = TimeSpec::deserialize(settings.value("start").toStringList());
		TimeSpec end = TimeSpec::deserialize(settings.value("end").toStringList());
		addChoice(start, end);
	}
	settings.endArray();

	if (size < 1)
	{
		addChoice(TimeSpec(15, TimeSpec::Minutes), TimeSpec());
		addChoice(TimeSpec(1, TimeSpec::Hours), TimeSpec());
		addChoice(TimeSpec(4, TimeSpec::Hours), TimeSpec());
		addChoice(TimeSpec(1, TimeSpec::Days), TimeSpec());
		addChoice(TimeSpec(1, TimeSpec::Weeks), TimeSpec());
		addChoice(TimeSpec(1, TimeSpec::Months), TimeSpec());
		addChoice(TimeSpec(1, TimeSpec::Years), TimeSpec());
	}
}


int TimerangeModel::rowCount(const QModelIndex& parent) const
{
	return m_data.size() +1;
}


QVariant TimerangeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (section == 0 && orientation == Qt::Horizontal && role == Qt::DisplayRole)
		return "Time Range";
	return QVariant();
}


QVariant TimerangeModel::data(const QModelIndex& index, int role) const
{
	QVariant result;
	if (role == Qt::DisplayRole && index.row() >= 0)
	{
		if (index.row() < m_data.size())
		{
			auto data = m_data[index.row()];
			result.setValue(QString("%1 to %2").arg(data.first.toString(), data.second.toString()));
		}
		else
			result.setValue(QString("Custom ..."));
	}
	else if (role == Qt::UserRole && index.row() >= 0 && index.row() < m_data.size())
		result.setValue(m_data[index.row()]);
	return result;
}


int TimerangeModel::addChoice(const TimeSpec& start, const TimeSpec& end)
{
	auto entry = qMakePair(start, end);
	if (m_data.contains(entry))
	{
		return m_data.indexOf(entry);
	}
	else
	{
		beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
		m_data.append(qMakePair(start, end));
		endInsertRows();
		return m_data.size() -1;
	}
}
