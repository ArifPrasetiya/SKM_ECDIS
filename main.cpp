#include <QtGui/QApplication>
#include <base/inc/platform.h>
#include <base/inc/base_library/base_types_functions.h>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);

#if defined(SDK_OS_POSIX)
  a.setAttribute(Qt::AA_X11InitThreads, true);
#endif

  int res = 0;
  {
    MainWindow w;
    w.show();

    res = a.exec();
  }

  SDKShutdown();
  return res;
}
