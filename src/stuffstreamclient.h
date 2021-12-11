#ifndef STUFFSTREAMCLIENT_H
#define STUFFSTREAMCLIENT_H

#include <QObject>
#include <QSslConfiguration>
#include <QUrlQuery>
class QNetworkAccessManager;
class QNetworkReply;

class StuffstreamClient : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString baseUrl MEMBER m_baseUrl WRITE setBaseUrl);
	Q_PROPERTY(QString trustedCertificates MEMBER m_trustedCerts WRITE setTrustedCertificates);
	Q_PROPERTY(quint64 maxEvents MEMBER m_maxEvents WRITE setMaxEvents);

public:
	static StuffstreamClient* get();

	const QString& baseUrl() const { return m_baseUrl; }
	void setBaseUrl(const QString& url) { m_baseUrl = url; }
	const QString& trustedCertificates() const { return m_trustedCerts; }
	void setTrustedCertificates(const QString& filename);
	quint64 maxEvents() const { return m_maxEvents; }
	void setMaxEvents(quint64 max) { m_maxEvents = max; }

	QUrlQuery buildQueryParams(const QDateTime& start, const QDateTime& end, const QString& query) const;

public slots:
	QNetworkReply* fetchEvents(const QDateTime& start, const QDateTime& end, const QString& query);
	QNetworkReply* fetchCounts(const QDateTime& start, const QDateTime& end, const QString& query, const QString& splitBy, quint32 limitBuckets, const QString& metric, const QString& aggregate);

protected:
	QNetworkReply* sendRequest(const QUrl& url);

protected slots:
	void handleReply(QNetworkReply* reply);

signals:
	void receivedFields(QVariantMap keys);
	void receivedEvents(QVariantList events);
	void receivedMetadata(quint64 eventCount, quint64 fetchedEvents);
	void receivedCounts(QVariantMap counts);
	void requestError(QString error);

	void sslErrors(QNetworkReply* reply, const QList<QSslError>& errors);
	void requestStarted(QNetworkReply* reply);
	void requestFinished(QNetworkReply* reply);

private:
	explicit StuffstreamClient(QObject* parent = nullptr);

	QSslConfiguration m_sslConfig;
	QNetworkAccessManager* m_netAccess;
	QString m_baseUrl;
	QString m_trustedCerts;
	quint64 m_maxEvents;
};

#endif // STUFFSTREAMCLIENT_H
