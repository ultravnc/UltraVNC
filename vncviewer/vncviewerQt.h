/*
 * mainwindows Qt Headers vncviewerQt.h from vncviewer.h
*/


#ifndef VNCVIEWERQT_H
#define VNCVIEWERQT_H

#include <QMainWindow>

namespace Ui {

class VNCViewerQt;
}

class VNCViewerQt : public QMainWindow
{
    Q_OBJECT

public:
    explicit VNCViewerQt(QWidget *parent = nullptr);
    ~VNCViewerQt();

private:
    Ui::VNCViewerQt *ui;
};

#endif // VNCVIEWERQT_H

/*
*
* Original code NotQt below
*
*/
// #ifndef VNCVIEWER_H__
// #define VNCVIEWER_H__

// #pragma once

// #include "res/resource.h"
// #include "VNCviewerApp.h"
// #include "Log.h"
// #include "AccelKeys.h"

// #define WM_SOCKEVENT WM_APP+1
// #define WM_TRAYNOTIFY WM_SOCKEVENT+1
// //adzm 2010-09
// #define WM_REQUESTUPDATE WM_TRAYNOTIFY+1
// // WM_REQUESTUPDATE (wParam, lParam)
// // wParam:
// //		0x00000000 = Full Framebuffer Update Request
// //		0x00000001 = Incremental Framebuffer Update Request
// //		0xFFFFFFFF = 'Appropriate' Framebuffer Update Request
// //adzm 2010-09
// #define WM_UPDATEREMOTECLIPBOARDCAPS WM_REQUESTUPDATE+1
// #define WM_UPDATEREMOTECLIPBOARD WM_REQUESTUPDATE+2
// #define WM_NOTIFYPLUGINSTREAMING WM_REQUESTUPDATE+3
// //adzm 2010-09
// #define WM_SENDKEEPALIVE WM_NOTIFYPLUGINSTREAMING+1
// #define WM_SOCKEVENT6 WM_SENDKEEPALIVE+1
// #define WM_SOCKEVENT4 WM_SENDKEEPALIVE+2

// // The Application
// extern VNCviewerApp *pApp;

// // Global logger - may be used by anything
// extern Log vnclog;
// extern AccelKeys TheAccelKeys;

// // Display given window in centre of screen
// void CentreWindow(HWND hwnd);

// // Convert "host:display" into host and port
// // Returns true if valid.
// bool ParseDisplay(LPTSTR display, LPTSTR phost, int hostlen, int *port);

// // Macro DIALOG_MAKEINTRESOURCE is used to allow both normal windows dialogs
// // and the selectable aspect ratio dialogs under WinCE (PalmPC vs HPC).
// #define DIALOG_MAKEINTRESOURCE MAKEINTRESOURCE
// #endif

