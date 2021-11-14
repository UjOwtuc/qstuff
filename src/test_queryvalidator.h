#ifndef TEST_QUERYVALIDATOR_H
#define TEST_QUERYVALIDATOR_H

#include <QTest>

class TestQueryValidator : public QObject
{
	Q_OBJECT
private slots:
	void fields_data();
	void fields();

	void scalar_data();
	void scalar();

	void list_data();
	void list();
};


#endif // TEST_QUERYVALIDATOR_H
