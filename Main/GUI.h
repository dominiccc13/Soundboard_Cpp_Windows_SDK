#pragma once
#include "WasapiMixer.h"
#include "Utils.h"
#include "Tray.h"
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QWidget>
#include <QThread>
#include <QMutex>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>

class SoundWorker : public QObject {
    Q_OBJECT
public:
    SoundWorker(QObject *parent = nullptr);
    ~SoundWorker();

public slots:
    void setBasePath(const QString &path);
    void updateSoundbiteFile(int index, const QString &filename);
    void startEngine();
    void stopEngine();

signals:
    void engineStopped();

private:
    std::atomic<bool> running;
    QMutex mutex;
    QString basePath;
    std::vector<std::string> soundbiteFiles;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void requestSetBasePath(const QString &path);
    void requestUpdateSoundbite(int index, const QString &filename);
    void requestStartEngine();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onBrowseClicked();
    void onAddRowClicked();
    void onChangeSoundbiteClicked(int row);
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void addRow(char hotkey, const QString &filename);

    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
    
    QHBoxLayout *topLayout;
    QLineEdit *pathLineEdit;
    QPushButton *browseButton;

    QVBoxLayout *rowsLayout;
    QWidget *rowsContainer;
    QScrollArea *scrollArea;

    QPushButton *addRowButton;

    QString hotkeySequence;
    int currentRowIndex;

    struct SoundRow {
        QLabel *hotkeyLabel;
        QLineEdit *fileLineEdit;
        QPushButton *changeButton;
    };

    std::vector<SoundRow> soundRows;

    QThread *workerThread;
    SoundWorker *soundWorker;

    // System Tray integration
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
};