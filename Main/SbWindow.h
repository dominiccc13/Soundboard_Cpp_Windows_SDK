#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <functional>

// Matches g_soundbiteCount in WasapiMixer.h (mixer has fixed slots)
inline constexpr int kSbMaxEntries = 20;

// A soundbite: UTF-8 path to a .wav file + the key used with Ctrl+Shift+Alt (0-9, A-Z)
// slot is the mixer slot; it stays fixed when other soundbites are removed
struct SbEntry {
    std::string path;
    char hotkey;
    int slot;
};

// Called (from the GUI thread) after the user adds, edits, or removes a soundbite
inline std::function<void()> g_OnSoundbitesChanged = nullptr;

void SbWindowInit(const std::vector<SbEntry>& initial); // starts the Qt GUI thread
void SbWindowShow();                                    // thread-safe, assign to g_OnTrayDoubleClick
void SbWindowShutdown();                                // quits Qt and joins the GUI thread

std::vector<SbEntry> SbGetEntries();                    // thread-safe copy
int SbFindByHotkey(char hotkey);                        // mixer slot for hotkey, or -1
