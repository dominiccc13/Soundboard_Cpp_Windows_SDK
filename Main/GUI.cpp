#include "GUI.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QDebug>
#include <QApplication>
#include <QStyle>
#include <QTimer>

Soundboard::Soundboard(QObject *parent) : QObject(parent) {}

Soundboard::~Soundboard() {
    stopSoundboard();
}

void Soundboard::setBasePath(const QString &path) {
    QMutexLocker locker(&mutex);
    basePath = path;
}

void Soundboard::updateSoundbiteFile(int index, const QString &filename) {
    QMutexLocker locker(&mutex);
    if (index >= 0 && index < static_cast<int>(soundbiteFiles.size())) {
        soundbiteFiles[index] = filename.toStdString();
    } else {
        while (static_cast<int>(soundbiteFiles.size()) <= index) {
            soundbiteFiles.push_back("");
        }
        soundbiteFiles[index] = filename.toStdString();
    }
}

void Soundboard::startSoundboard() {
    WasapiMixer mixer;
    int soundbiteIndex = -1;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    {
        QMutexLocker locker(&mutex);
        std::string bPath = basePath.toStdString();
        if (bPath.empty()) bPath = "C:\\Users\\Public\\cpp_Soundboard\\Resources\\Test_Soundbites\\";
        if (bPath.back() != '\\' && bPath.back() != '/') bPath += "\\";

        for (size_t i = 0; i < soundbiteFiles.size(); i++) {
            if (!soundbiteFiles[i].empty()) {
                std::vector<float> soundbiteData = LoadWavFile(bPath + soundbiteFiles[i]);
                mixer.LoadSoundbite(static_cast<int>(i), soundbiteData);
            }
        }
    }

    if (!mixer.Start(L"CABLE Input")) {
        std::cerr << "Failed to start WasapiMixer! Make sure VB-Cable is active.\n";
        CoUninitialize();
        g_running = false;
        return;
    }

    int count = static_cast<int>(soundbiteFiles.size());
    g_soundbiteNames.resize(count);
    g_soundbiteKeys.resize(count);
    for (int i = 0; i < count; i++) {
        std::string sPath = soundbiteFiles[i];
        if (sPath.length() > 4) sPath = sPath.substr(0, sPath.length() - 4);
        std::wstring w_sPath = StringToWString(sPath);
        g_soundbiteNames[i] = w_sPath;
    }
    std::thread trayThread(TrayLoop);

    const std::string hotkeys = "1234567890qwertyuioasdfghjklzxcvbnm";

    while (g_running.load()) {
        bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

        if (ctrl && shift && alt) {
            char pressedKey = GetPressedAlphaNumericKey();
            if (pressedKey == '\0') {
                std::this_thread::sleep_for(std::chrono::milliseconds(15));
                continue;
            }

            if (pressedKey == 'Q' || pressedKey == 'q') {
                g_running.store(false);
                break;
            }

            int foundIndex = -1;
            for (size_t i = 0; i < hotkeys.length(); ++i) {
                if (toupper(hotkeys[i]) == toupper(pressedKey)) {
                    foundIndex = static_cast<int>(i);
                    break;
                }
            }
            if (foundIndex >= 0 && foundIndex < static_cast<int>(soundbiteFiles.size())) {
                mixer.TriggerSoundbite(foundIndex);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    mixer.Stop();
    CoUninitialize();

    if (g_trayThreadId != 0) {
        PostThreadMessage(g_trayThreadId, WM_QUIT, 0, 0);
    }
    if (trayThread.joinable()) {
        trayThread.join();
    }

    emit soundboardStopped();
}

void Soundboard::stopSoundboard() {
    g_running.store(false);
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), currentRowIndex(0) {
    hotkeys = "1234567890qwertyuioasdfghjklzxcvbnm";

    // ui elements
    centralWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(centralWidget);
    topLayout = new QHBoxLayout();
    QLabel *pathLabel = new QLabel("Soundbites Dir:", this);
    pathLineEdit = new QLineEdit(this);
    pathLineEdit->setText("C:\\Users\\Public\\cpp_Soundboard\\Resources\\Test_Soundbites\\");
    browseButton = new QPushButton("Browse...", this);
    topLayout->addWidget(pathLabel);
    topLayout->addWidget(pathLineEdit);
    topLayout->addWidget(browseButton);
    mainLayout->addLayout(topLayout);
    rowsContainer = new QWidget(this);
    rowsLayout = new QVBoxLayout(rowsContainer);
    rowsLayout->setAlignment(Qt::AlignTop);
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(rowsContainer);
    mainLayout->addWidget(scrollArea);
    addRowButton = new QPushButton("Add Row", this);
    mainLayout->addWidget(addRowButton);
    setCentralWidget(centralWidget);
    resize(550, 400);
    setWindowTitle("Qt 6.11.2 Soundboard Manager");

    // instantiate tray double click method
    g_OnTrayDoubleClick = [this]() {
        QMetaObject::invokeMethod(this, [this]() {
            show();
            setWindowState(windowState() & ~Qt::WindowMinimized);
            raise();
            activateWindow();
        }, Qt::QueuedConnection);
    };
    
    // connect signals
    connect(browseButton, &QPushButton::clicked, this, &MainWindow::onBrowseClicked);
    connect(addRowButton, &QPushButton::clicked, this, &MainWindow::onAddRowClicked);
    connect(pathLineEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        emit requestSetBasePath(text);
    });
    QTimer *quitChecker = new QTimer(this);
    connect(quitChecker, &QTimer::timeout, this, [this]() {
        if (!g_running.load()) QCoreApplication::quit();
    });
    quitChecker->start(100);

    // instantiate and connect soundboard on background qthread "soundboardThread"
    soundboardThread = new QThread(this);
    soundboard = new Soundboard();
    soundboard->moveToThread(soundboardThread);
    connect(soundboardThread, &QThread::finished, soundboard, &QObject::deleteLater);
    connect(this, &MainWindow::requestSetBasePath, soundboard, &Soundboard::setBasePath);
    connect(this, &MainWindow::requestUpdateSoundbite, soundboard, &Soundboard::updateSoundbiteFile);
    connect(this, &MainWindow::requestStartSoundboard, soundboard, &Soundboard::startSoundboard);
    soundboardThread->start();

    // initialize with default rows
    QStringList defaultFiles = {"1.wav", "2.wav", "3.wav", "4.wav", "5.wav"};
    for (int i = 0; i < 5; ++i) {
        if (currentRowIndex < hotkeys.length()) {
            QString filename = (i < defaultFiles.size()) ? defaultFiles[i] : "";
            addRow(hotkeys[currentRowIndex].toLatin1(), filename);
        }
    }

    emit requestSetBasePath(pathLineEdit->text());
    emit requestStartSoundboard();
}

MainWindow::~MainWindow() {
    soundboard->stopSoundboard();
    soundboardThread->quit();
    soundboardThread->wait();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    hide();
    event->ignore();
}

void MainWindow::onBrowseClicked() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Soundbites Directory",
                                                    pathLineEdit->text());
    if (!dir.isEmpty()) {
        pathLineEdit->setText(dir);
    }
}

void MainWindow::onAddRowClicked() {
    if (currentRowIndex < hotkeys.length()) {
        addRow(hotkeys[currentRowIndex].toLatin1(), "");
    }
}

void MainWindow::onChangeSoundbiteClicked(int row) {
    QString startDir = pathLineEdit->text();
    QString filePath = QFileDialog::getOpenFileName(this, "Select Soundbite",
                                                   startDir, "WAV Files (*.wav)");
    if (!filePath.isEmpty()) {
        QFileInfo fileInfo(filePath);
        soundbiteRows[row].fileLineEdit->setText(fileInfo.fileName());
        emit requestUpdateSoundbite(row, fileInfo.fileName());
    }
}

void MainWindow::addRow(char hotkey, const QString &filename) {
    QHBoxLayout *rowLayout = new QHBoxLayout();

    QLabel *hotkeyLabel = new QLabel(QString("[%1]").arg(hotkey), this);
    hotkeyLabel->setFixedWidth(40);

    QLineEdit *fileLineEdit = new QLineEdit(filename, this);
    fileLineEdit->setReadOnly(true);
    fileLineEdit->setPlaceholderText("No wav file selected");

    QPushButton *changeButton = new QPushButton("Change Soundbite", this);

    rowLayout->addWidget(hotkeyLabel);
    rowLayout->addWidget(fileLineEdit);
    rowLayout->addWidget(changeButton);

    rowsLayout->addLayout(rowLayout);

    int index = static_cast<int>(soundbiteRows.size());
    soundbiteRows.push_back({hotkeyLabel, fileLineEdit, changeButton});
    currentRowIndex++;

    if (!filename.isEmpty()) {
        emit requestUpdateSoundbite(index, filename);
    }

    connect(changeButton, &QPushButton::clicked, this, [this, index]() {
        onChangeSoundbiteClicked(index);
    });
}

// void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
//     if (reason == QSystemTrayIcon::DoubleClick) {
//         if (!isVisible()) {
//             show();
//             activateWindow();
//         }
//     }
// }