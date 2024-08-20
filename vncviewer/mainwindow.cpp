#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include "AboutBoxQt.h" //Optional if done in .h

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    windowsvncviewerQt = new AboutBoxQt(this);

    //Action close on button leave
    //To Do if needed

    //Action call new windows with an executable
    connect(ui->actionAbout,SIGNAL(triggered(bool)),this,SLOT(AboutVNCViewerQt()));

}

MainWindow::~MainWindow()
{
    delete ui;
}

//void windowsvncviewerQt::AboutBoxQt()
void MainWindow::AboutBoxQt( )
{
    //Active only one of the three line below
    //AboutBoxQt *windowsvncviewerQt = new AboutBoxQt(this);
    //windowsvncviewerQt->exec();  // for blocking dialogue
    windowsvncviewerQt->show();    // for a non-blocking dialogue
}




// Code auto-generated from template
// #include "mainwindow.h"
// #include "ui_mainwindow.h"

// MainWindow::MainWindow(QWidget *parent)
//     : QMainWindow(parent)
//     , ui(new Ui::MainWindow)
// {
//     ui->setupUi(this);
// }

// MainWindow::~MainWindow()
// {
//     delete ui;
// }
