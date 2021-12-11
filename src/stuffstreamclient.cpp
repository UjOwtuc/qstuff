#include "stuffstreamclient.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace {
	StuffstreamClient* instance = nullptr;
}

StuffstreamClient::StuffstreamClient(QObject* parent)
	: QObject(parent),
	m_sslConfig(QSslConfiguration::defaultConfiguration())
{
	m_netAccess = new QNetworkAccessManager(this);
	connect(m_netAccess, &QNetworkAccessManager::sslErrors, this, &StuffstreamClient::sslErrors);
}


StuffstreamClient * StuffstreamClient::get()
{
	if (! instance)
		instance = new StuffstreamClient();
	return instance;
}


void StuffstreamClient::setTrustedCertificates(const QString& filename)
{
	if (filename != m_trustedCerts || filename.isEmpty())
	{
		m_trustedCerts = filename;
		m_sslConfig = QSslConfiguration::defaultConfiguration();
		if (! filename.isEmpty())
			m_sslConfig.addCaCertificates(QSslCertificate::fromPath(filename));
	}
}


QUrlQuery StuffstreamClient::buildQueryParams(const QDateTime& start, const QDateTime& end, const QString& query) const
{
	QUrlQuery queryItems;
	queryItems.addQueryItem("start", start.toUTC().toString(Qt::ISODate));
	queryItems.addQueryItem("end", end.toUTC().toString(Qt::ISODate));

	if (! query.isEmpty())
		queryItems.addQueryItem("query", query);
	return queryItems;
}


QNetworkReply* StuffstreamClient::fetchEvents(const QDateTime& start, const QDateTime& end, const QString& query)
{
	QUrlQuery queryItems = buildQueryParams(start, end, query);
	queryItems.addQueryItem("limit_events", QString::number(m_maxEvents));
	QUrl url(m_baseUrl);
	url.setPath("/events");
	url.setQuery(queryItems);
	qDebug() << "search URL:" << url;
	return sendRequest(url);
}


QNetworkReply* StuffstreamClient::fetchCounts(const QDateTime& start, const QDateTime& end, const QString& query, const QString& splitBy, quint32 limitBuckets, const QString& metric, const QString& aggregate)
{
	QUrlQuery queryItems = buildQueryParams(start, end, query);
	if (! splitBy.isEmpty())
		queryItems.addQueryItem("split_by", splitBy);
	if (limitBuckets > 0)
		queryItems.addQueryItem("max_buckets", QString::number(limitBuckets));
	if (! metric.isEmpty())
		queryItems.addQueryItem("value", metric);
	if (! aggregate.isEmpty())
		queryItems.addQueryItem("aggregate", aggregate);

	QUrl url(m_baseUrl);
	url.setPath("/counts");
	url.setQuery(queryItems);
	qDebug() << "counts URL:" << url;
	return sendRequest(url);
}


QNetworkReply* StuffstreamClient::sendRequest(const QUrl& url)
{
	QNetworkRequest req(url);
	req.setSslConfiguration(m_sslConfig);
	auto reply = m_netAccess->get(req);
	connect(reply, &QNetworkReply::finished, this, [this,reply]{
		handleReply(reply);
	});
	emit requestStarted(reply);
	return reply;
}


void StuffstreamClient::handleReply(QNetworkReply* reply)
{
	emit requestFinished(reply);
	auto error = reply->error();
	if (error != QNetworkReply::NoError)
	{
		emit requestError(QString("Network request failed: %1").arg(reply->errorString()));
		return;
	}

	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
	if (parseError.error != QJsonParseError::NoError)
	{
		emit requestError(QString("Could not parse server's reply: %1").arg(parseError.errorString()));
		return;
	}

	QVariantMap map(doc.object().toVariantMap());
	quint64 numEvents, matchedEvents;
	auto end = map.constKeyValueEnd();
	for (auto it=map.constKeyValueBegin(); it!=end; ++it)
	{
		if (it->first == "fields")
			emit receivedFields(it->second.toMap());
		else if (it->first == "events")
		{
			auto events = it->second.toList();
			matchedEvents = events.size();
			emit receivedEvents(events);
		}
		else if (it->first == "metadata")
		{
			auto meta = it->second.toMap();
			numEvents = meta["event_count"].toULongLong();
		}
		else if (it->first == "counts")
			emit receivedCounts(it->second.toMap());
		else
		{
			qWarning() << "unknown item in reply:" << it->first << it->second;
		}
	}
	if (numEvents)
		emit receivedMetadata(numEvents, matchedEvents);
}
