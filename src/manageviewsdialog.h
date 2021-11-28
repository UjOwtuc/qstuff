#ifndef MANAGEVIEWSDIALOG_H
#define MANAGEVIEWSDIALOG_H

#include <QDialog>
namespace Ui {
	class ManageViewsDialog;
}
class SavedViewsModel;

class ManageViewsDialog : public QDialog
{
	Q_OBJECT
public:
	explicit ManageViewsDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
	SavedViewsModel* savedViewsModel() { return m_viewsModel; }

public slots:
	void removeViews();
	void editView();

private:
	Ui::ManageViewsDialog* m_widget;
	SavedViewsModel* m_viewsModel;
};

#endif // MANAGEVIEWSDIALOG_H
