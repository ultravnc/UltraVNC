#ifndef ABOUTBOXQT_H
#define ABOUTBOXQT_H

#include <QDialog>

namespace Ui {
class AboutBoxQt;
}

class AboutBoxQt : public QDialog
{
    Q_OBJECT

public:
    explicit AboutBoxQt(QWidget *parent = nullptr);
    ~AboutBoxQt();

private:
    Ui::AboutBoxQt *ui;
};

#endif // ABOUTBOXQT_H

/*
*
* Original code NotQt below
*
*/
// // Display the about box!

// void ShowAboutBox();
