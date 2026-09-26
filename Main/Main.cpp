#define NOMINMAX
#include "WasapiMixer.h"
#include "Utils.h"
#include "Tray.h"
#include "GUI.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    MainWindow window;
    
    return app.exec();
}