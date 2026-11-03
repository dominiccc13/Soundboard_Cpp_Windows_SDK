#include "SbWindow.h"
#include "Tray.h"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <atomic>
#include <future>
#include <mutex>
#include <thread>

namespace {

std::mutex g_mutex;
std::vector<SbEntry> g_entries;
std::thread g_guiThread;
std::atomic<QObject*> g_window{ nullptr };

std::string Stem(const std::string& path) {
    return QFileInfo(QString::fromStdString(path)).completeBaseName().toStdString();
}

// keep the tray's left-click menu in sync with g_entries (caller holds g_mutex)
void RefreshTray() {
    g_soundbiteKeys.clear();
    g_soundbiteNames.clear();
    for (const SbEntry& e : g_entries) {
        std::wstring key = std::wstring(1, (wchar_t)e.hotkey) + L" : ";
        g_soundbiteKeys.push_back(key);
        g_soundbiteNames.push_back(key + QString::fromStdString(Stem(e.path)).toStdWString());
    }
}

void Commit(const std::vector<SbEntry>& entries) {
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_entries = entries;
        RefreshTray();
    }
    if (g_OnSoundbitesChanged) g_OnSoundbitesChanged();
}

// add / edit form: wav path + hotkey
class EntryEditor : public QDialog {
public:
    EntryEditor(const QString& title, const SbEntry& initial, const QString& usedKeys, QWidget* parent)
        : QDialog(parent), slot(initial.slot) {
        setWindowTitle(title);
        setMinimumWidth(480);

        pathEdit = new QLineEdit(QString::fromStdString(initial.path));
        auto* browse = new QPushButton("Browse...");
        hotkeyBox = new QComboBox;
        for (char c = '0'; c <= 'Z'; c++) {
            if (c > '9' && c < 'A') continue;
            if (c == 'P') continue; // reserved: exits the app
            if (usedKeys.contains(c) && c != initial.hotkey) continue;
            hotkeyBox->addItem(QString(c));
        }
        if (initial.hotkey) hotkeyBox->setCurrentText(QString(initial.hotkey));

        auto* pathRow = new QHBoxLayout;
        pathRow->addWidget(pathEdit);
        pathRow->addWidget(browse);
        auto* hotkeyRow = new QHBoxLayout;
        hotkeyRow->addWidget(new QLabel("Hotkey: Ctrl+Shift+Alt+"));
        hotkeyRow->addWidget(hotkeyBox);
        hotkeyRow->addStretch();
        auto* ok = new QPushButton("OK");
        auto* cancel = new QPushButton("Cancel");
        ok->setDefault(true);
        auto* buttons = new QHBoxLayout;
        buttons->addStretch();
        buttons->addWidget(ok);
        buttons->addWidget(cancel);

        auto* layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel("Path to .wav file:"));
        layout->addLayout(pathRow);
        layout->addLayout(hotkeyRow);
        layout->addLayout(buttons);

        connect(browse, &QPushButton::clicked, this, [this] {
            QString file = QFileDialog::getOpenFileName(this, "Select soundbite", pathEdit->text(), "WAV files (*.wav)");
            if (!file.isEmpty()) pathEdit->setText(QDir::toNativeSeparators(file));
        });
        connect(ok, &QPushButton::clicked, this, [this] {
            QFileInfo info(pathEdit->text());
            if (!info.isFile() || info.suffix().compare("wav", Qt::CaseInsensitive) != 0) {
                QMessageBox::warning(this, "Soundboard", "Path must point to an existing .wav file.");
                return;
            }
            accept();
        });
        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    }

    SbEntry result() const {
        return { pathEdit->text().toStdString(), hotkeyBox->currentText().at(0).toLatin1(), slot };
    }

private:
    int slot;
    QLineEdit* pathEdit;
    QComboBox* hotkeyBox;
};

// window icon: logoR.ico, embedded in the exe by resource.rc as IDI_MYICON
QIcon WindowIcon() {
    QIcon icon;
    for (int size : { 16, 32 }) {
        HICON hIcon = (HICON)LoadImageW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDI_MYICON), IMAGE_ICON, size, size, 0);
        if (!hIcon) continue;
        icon.addPixmap(QPixmap::fromImage(QImage::fromHICON(hIcon)));
        DestroyIcon(hIcon);
    }
    return icon;
}

// main window: soundbite | path | hotkey + Add / Edit / Remove
class SbDialog : public QDialog {
public:
    SbDialog() {
        setWindowTitle("Soundboard");
        setWindowIcon(WindowIcon());
        resize(720, 420);

        table = new QTableWidget(0, 3);
        table->setHorizontalHeaderLabels({ "Soundbite", "Path", "Hotkey" });
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->verticalHeader()->setVisible(false);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

        auto* add = new QPushButton("Add");
        auto* edit = new QPushButton("Edit");
        auto* remove = new QPushButton("Remove");
        auto* buttons = new QHBoxLayout;
        buttons->addStretch();
        buttons->addWidget(add);
        buttons->addWidget(edit);
        buttons->addWidget(remove);

        auto* layout = new QVBoxLayout(this);
        layout->addWidget(table);
        layout->addLayout(buttons);

        connect(add, &QPushButton::clicked, this, [this] { onAdd(); });
        connect(edit, &QPushButton::clicked, this, [this] { onEdit(); });
        connect(table, &QTableWidget::cellDoubleClicked, this, [this] { onEdit(); });
        connect(remove, &QPushButton::clicked, this, [this] { onRemove(); });
        refresh();
    }

    void showAndRaise() {
        refresh();
        show();
        raise();
        activateWindow();
    }

private:
    QString usedKeys() const {
        QString keys;
        for (const SbEntry& e : SbGetEntries()) keys += QChar(e.hotkey);
        return keys;
    }

    void refresh() {
        std::vector<SbEntry> entries = SbGetEntries();
        table->setRowCount((int)entries.size());
        for (int i = 0; i < (int)entries.size(); i++) {
            table->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(Stem(entries[i].path))));
            table->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(entries[i].path)));
            table->setItem(i, 2, new QTableWidgetItem(QString("Ctrl+Shift+Alt+") + QChar(entries[i].hotkey)));
        }
    }

    void onAdd() {
        std::vector<SbEntry> entries = SbGetEntries();
        if ((int)entries.size() >= kSbMaxEntries) {
            QMessageBox::information(this, "Soundboard", "Maximum number of soundbites reached.");
            return;
        }
        // lowest mixer slot not used by an existing soundbite
        int slot = 0;
        for (bool taken = true; taken; slot++) {
            taken = false;
            for (const SbEntry& e : entries) taken = taken || e.slot == slot;
        }
        EntryEditor editor("Add soundbite", { "", 0, slot - 1 }, usedKeys(), this);
        if (editor.exec() != QDialog::Accepted) return;
        entries.push_back(editor.result());
        Commit(entries);
        refresh();
    }

    void onEdit() {
        int row = table->currentRow();
        std::vector<SbEntry> entries = SbGetEntries();
        if (row < 0 || row >= (int)entries.size()) return;
        EntryEditor editor("Edit soundbite", entries[row], usedKeys(), this);
        if (editor.exec() != QDialog::Accepted) return;
        entries[row] = editor.result();
        Commit(entries);
        refresh();
    }

    void onRemove() {
        int row = table->currentRow();
        std::vector<SbEntry> entries = SbGetEntries();
        if (row < 0 || row >= (int)entries.size()) return;
        entries.erase(entries.begin() + row);
        Commit(entries);
        refresh();
    }

    QTableWidget* table;
};

void GuiThread(std::promise<void> ready) {
    static int argc = 1;
    static char arg0[] = "soundboard";
    static char* argv[] = { arg0, nullptr };
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false); // closing the window only hides it; tray keeps app alive

    SbDialog window;
    g_window.store(&window);
    ready.set_value();
    app.exec();
    g_window.store(nullptr);
}

} // namespace

void SbWindowInit(const std::vector<SbEntry>& initial) {
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_entries = initial;
        RefreshTray();
    }
    std::promise<void> ready;
    std::future<void> readyFuture = ready.get_future();
    g_guiThread = std::thread(GuiThread, std::move(ready));
    readyFuture.wait();
}

void SbWindowShow() {
    QObject* window = g_window.load();
    if (!window) return;
    QMetaObject::invokeMethod(window, [window] {
        static_cast<SbDialog*>(window)->showAndRaise();
    }, Qt::QueuedConnection);
}

void SbWindowShutdown() {
    if (QObject* window = g_window.load()) {
        QMetaObject::invokeMethod(window, [] { QApplication::quit(); }, Qt::QueuedConnection);
    }
    if (g_guiThread.joinable()) g_guiThread.join();
}

std::vector<SbEntry> SbGetEntries() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_entries;
}

int SbFindByHotkey(char hotkey) {
    std::lock_guard<std::mutex> lock(g_mutex);
    for (const SbEntry& e : g_entries) {
        if (e.hotkey == hotkey) return e.slot;
    }
    return -1;
}
