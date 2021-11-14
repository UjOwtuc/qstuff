#include "test_queryvalidator.h"
#include "queryvalidator.h"

Q_DECLARE_METATYPE(QValidator::State);


void TestQueryValidator::fields_data()
{
	QTest::addColumn<QString>("input");
	QTest::addColumn<QValidator::State>("result");

	QTest::newRow("empty name") << "" << QValidator::Intermediate;
	QTest::newRow("alpha") << "abcABC" << QValidator::Acceptable;
	QTest::newRow("alphanumeric") << "abcABC123" << QValidator::Acceptable;
	QTest::newRow("minus") << "a-bc" << QValidator::Acceptable;
	QTest::newRow("underscore") << "a_bc" << QValidator::Acceptable;
	QTest::newRow("dot") << "a.bc" << QValidator::Acceptable;
	QTest::newRow("starting digit") << "0abc" << QValidator::Intermediate;
	QTest::newRow("space") << "a bc" << QValidator::Intermediate;
}


void TestQueryValidator::fields()
{
	QFETCH(QString, input);
	QFETCH(QValidator::State, result);

	int pos = 0;
	QueryValidator validator(QueryValidator::Field);
	QCOMPARE(validator.validate(input, pos), result);
}


void TestQueryValidator::scalar_data()
{
	QTest::addColumn<QString>("input");
	QTest::addColumn<QValidator::State>("result");

	QTest::newRow("integer") << "123" << QueryValidator::Acceptable;
	QTest::newRow("float") << "0.123" << QueryValidator::Acceptable;
	QTest::newRow("quoted string") << "\"abc def\"" << QueryValidator::Acceptable;

	QTest::newRow("unquoted string") << "abc" << QueryValidator::Intermediate;
	QTest::newRow("single quote") << "\"abc" << QueryValidator::Intermediate;
	QTest::newRow("leading zero") << "01" << QueryValidator::Intermediate;
	QTest::newRow("multiple dots") << "0.12.3" << QueryValidator::Intermediate;
}


void TestQueryValidator::scalar()
{
	QFETCH(QString, input);
	QFETCH(QValidator::State, result);

	int pos = 0;
	QueryValidator validator(QueryValidator::Scalar);
	QCOMPARE(validator.validate(input, pos), result);
}


void TestQueryValidator::list_data()
{
	QTest::addColumn<QString>("input");
	QTest::addColumn<QValidator::State>("result");

	QTest::newRow("empty") << "()" << QueryValidator::Intermediate;
	QTest::newRow("integer") << "(1, 2, 3)" << QueryValidator::Acceptable;
	QTest::newRow("floats") << "(1.1, 2.2, 3.3)" << QueryValidator::Acceptable;
	QTest::newRow("mixed interger and floats") << "(1.1, 2, 3.3)" << QueryValidator::Acceptable;
	QTest::newRow("strings") << "(\"a\", \"b\", \"c\")" << QueryValidator::Acceptable;
	QTest::newRow("mixed numbers and strings") << "(\"a\", 5, \"c\")" << QueryValidator::Intermediate;
}


void TestQueryValidator::list()
{
	QFETCH(QString, input);
	QFETCH(QValidator::State, result);

	int pos = 0;
	QueryValidator validator(QueryValidator::List);
	QCOMPARE(validator.validate(input, pos), result);
}

QTEST_MAIN(TestQueryValidator)
