#pragma once
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <vector>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

private slots:
    void onBrowseClicked();
    void onAddRowClicked();
    void onChangeSoundbiteClicked(int row);

private:
    void addRow(char hotkey, const QString &filename = "");

    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
    
    // Top section
    QHBoxLayout *topLayout;
    QLineEdit *pathLineEdit;
    QPushButton *browseButton;

    // Rows container
    QVBoxLayout *rowsLayout;
    QWidget *rowsContainer;
    QScrollArea *scrollArea;

    // Add button
    QPushButton *addRowButton;

    // Hotkey progression
    QString hotkeySequence;
    int currentRowIndex;

    struct SoundRow {
        QLabel *hotkeyLabel;
        QLineEdit *fileLineEdit;
        QPushButton *changeButton;
    };

    std::vector<SoundRow> soundRows;
};