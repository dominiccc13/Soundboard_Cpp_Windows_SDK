// ===== Edit 1: INSERT after line 14 of Main.cpp (end of StringToWString) =====
// Exe-relative soundbite folder instead of a hardcoded absolute path.
static std::string ExeDirUtf8() {
    wchar_t buf[MAX_PATH];
    DWORD n = GetModuleFileNameW(NULL, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return std::string();
    std::wstring dir(buf, n);
    dir.resize(dir.find_last_of(L'\\') + 1);
    int len = WideCharToMultiByte(CP_UTF8, 0, dir.c_str(), (int)dir.size(), NULL, 0, NULL, NULL);
    std::string out(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, dir.c_str(), (int)dir.size(), &out[0], len, NULL, NULL);
    return out;
}

// ===== Edit 2: OVERWRITE line 114 of Main.cpp (soundbiteBasePath) =====
    std::string soundbiteBasePath = ExeDirUtf8() + "Soundbites\\";

// ===== Edit 3: INSERT as the first statement inside main(), before line 113 =====
    // only load DLLs from the app dir and System32 (blocks DLL hijacking via CWD/PATH)
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);

// ===== Edit 4: OVERWRITE lines 57-61 of Main.cpp (the "data" chunk branch) =====
// Don't trust the header's chunk size: cap at what's left in the file (avoids huge alloc / short read).
        else if (memcmp(subchunkId, "data", 4) == 0) {
            std::streampos pos = file.tellg();
            file.seekg(0, std::ios::end);
            std::streamoff remaining = file.tellg() - pos;
            file.seekg(pos);
            if (subchunkSize > static_cast<uint64_t>(remaining)) subchunkSize = static_cast<uint32_t>(remaining);
            rawAudioData.resize(subchunkSize);
            file.read(reinterpret_cast<char*>(rawAudioData.data()), subchunkSize);
            break;
        }
