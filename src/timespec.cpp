#include "timespec.h"

#include <QLocale>

QStringList TimeSpec::serialize() const
{
	QStringList result;
	result.append((m_kind == Absolute) ? "absolute" : "relative");
	if (m_kind == Absolute)
		result.append(m_absolute.toUTC().toString(Qt::ISODate));
	else
	{
		result << QString::number(m_relativeValue) << QString::number(m_relativeUnit);
	}
	return result;
}


TimeSpec TimeSpec::deserialize(const QStringList& s)
{
	if (s.first() == "absolute")
		return TimeSpec(QDateTime::fromString(s[1], Qt::ISODate));
	else
	{
		int val = s[1].toInt();
		Unit unit = static_cast<Unit>(s[2].toInt());
		return TimeSpec(val, unit);
	}
	qFatal("Unhandled timespec format: %s", s.first().toStdString().c_str());
}


bool TimeSpec::operator==(const TimeSpec& rhs) const
{
	if (m_kind == Absolute)
	{
		if (rhs.m_kind == Absolute && m_absolute == rhs.m_absolute)
			return true;
	}
	else
	{
		if (rhs.m_kind == m_kind && m_relativeUnit == rhs.m_relativeUnit && m_relativeValue == rhs.m_relativeValue)
			return true;
	}
	return false;
}


QString TimeSpec::toString() const
{
	QString s;
	if (m_kind == Absolute)
		s = QLocale().toString(m_absolute, QLocale::ShortFormat);
	else if (m_relativeValue == 0)
		s = "now";
	else
	{
		QString unit;
		switch (m_relativeUnit)
		{
			case Minutes:
				unit = "minutes";
				break;
			case Hours:
				unit = "hours";
				break;
			case Days:
				unit = "days";
				break;
			case Weeks:
				unit = "weeks";
				break;
			case Months:
				unit = "months";
				break;
			case Years:
				unit = "years";
				break;
		}
		s = QString("%1 %2 ago").arg(m_relativeValue).arg(unit);
	}
	return s;
}


QDateTime TimeSpec::toDateTime() const
{
	if (m_kind == Absolute)
		return m_absolute;

	QDateTime now = QDateTime::currentDateTime();
	return now.addSecs(-1 * static_cast<qint64>(m_relativeValue) * m_relativeUnit);
}

