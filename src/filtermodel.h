#ifndef FILTERMODEL_H
#define FILTERMODEL_H

#include <QAbstractListModel>

class FilterExpression
{
public:
	enum Op {Eq=0, Neq, Lt, Le, Gt, Ge, In, NotIn, Like, NotLike};

	static const QMap<Op, QString>& ops();
	static QString opString(Op op);
	static Op invertOp(Op op);

	FilterExpression();
	FilterExpression(const QString& id, Op op, const QString& value);
	FilterExpression(const FilterExpression& other);

	FilterExpression& operator=(const FilterExpression& rhs);
	bool operator<(const FilterExpression& rhs) const;
	bool operator==(const FilterExpression& rhs) const;
	bool operator!=(const FilterExpression& rhs) const;

	QString toString() const;
	FilterExpression inverted() const;
	bool enabled() const;
	void setEnabled(bool enabled);
	void setLabel(const QString& label);

	const QString& id() const { return m_id; }
	Op op() const { return m_op; }
	const QString& value() const { return m_value; }
	QString label() const;

	bool isEmpty() const;

private:
	QString m_id;
	Op m_op;
	QString m_value;
	bool m_enabled;
	QString m_label;
};
Q_DECLARE_METATYPE(FilterExpression);

class FilterModel : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit FilterModel(QObject* parent = nullptr);

	int rowCount(const QModelIndex & parent) const;
	QVariant data(const QModelIndex & index, int role) const;

	QStringList enabledExpressions() const;
	int findFilter(const FilterExpression& expr) const;
	int addFilter(const FilterExpression& expr);
	Qt::ItemFlags flags(const QModelIndex& index) const;
	bool setData(const QModelIndex & index, const QVariant & value, int role);
	const QList<FilterExpression>& filters();
	bool setAllEnabled(bool enabled);
	bool removeAllFilters();
	bool invertFilter(const QModelIndex& index);
	bool removeRows(int row, int count, const QModelIndex& /*parent*/);

signals:
	void checkStateChanged(const QModelIndex& index);

private:
	QList<FilterExpression> m_filters;
};

#endif // FILTERMODEL_H
