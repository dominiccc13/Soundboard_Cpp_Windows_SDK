#include "GUI.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentRowIndex(0) {
    
    hotkeySequence = "1234567890qwertyuioasdfghjklzxcvbnm";

    centralWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(centralWidget);

    topLayout = new QHBoxLayout();
    QLabel *pathLabel = new QLabel("Soundbites Dir:", this);
    pathLineEdit = new QLineEdit(this);
    pathLineEdit->setPlaceholderText("Select or enter folder path...");
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

    connect(browseButton, &QPushButton::clicked, this, &MainWindow::onBrowseClicked);
    connect(addRowButton, &QPushButton::clicked, this, &MainWindow::onAddRowClicked);

    for (int i = 0; i < 5; ++i) {
        if (currentRowIndex < hotkeySequence.length()) {
            addRow(hotkeySequence[currentRowIndex].toLatin1());
        }
    }
}

void MainWindow::onBrowseClicked() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Soundbites Directory",
                                                    pathLineEdit->text());
    if (!dir.isEmpty()) {
        pathLineEdit->setText(dir);
    }
}

void MainWindow::onAddRowClicked() {
    if (currentRowIndex < hotkeySequence.length()) {
        addRow(hotkeySequence[currentRowIndex].toLatin1());
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

    int index = static_cast<int>(soundRows.size());
    soundRows.push_back({hotkeyLabel, fileLineEdit, changeButton});
    currentRowIndex++;

    connect(changeButton, &QPushButton::clicked, this, [this, index]() {
        onChangeSoundbiteClicked(index);
    });
}

void MainWindow::onChangeSoundbiteClicked(int row) {
    QString startDir = pathLineEdit->text();
    QString filePath = QFileDialog::getOpenFileName(this, "Select Soundbite",
                                                   startDir, "WAV Files (*.wav)");
    if (!filePath.isEmpty()) {
        QFileInfo fileInfo(filePath);
        soundRows[row].fileLineEdit->setText(fileInfo.fileName());
    }
}