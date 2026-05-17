#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <windows.h>
#include <shlobj.h>

namespace fs = std::filesystem;

std::string wstring_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], size, nullptr, nullptr);
    return result;
}

std::wstring get_clipboard_path() {
    if (!OpenClipboard(nullptr)) return L"";
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData) {
        CloseClipboard();
        return L"";
    }
    wchar_t* pText = static_cast<wchar_t*>(GlobalLock(hData));
    std::wstring wstr(pText ? pText : L"");
    GlobalUnlock(hData);
    CloseClipboard();

    wstr.erase(wstr.find_last_not_of(L" \n\r\t\"'") + 1);
    wstr.erase(0, wstr.find_first_not_of(L" \n\r\t\"'"));
    return wstr;
}

bool set_clipboard_file(const std::wstring& path) {
    if (!OpenClipboard(nullptr)) return false;
    EmptyClipboard();

    size_t size = sizeof(DROPFILES) + (path.size() + 2) * sizeof(wchar_t);
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!hGlobal) {
        CloseClipboard();
        return false;
    }

    DROPFILES* df = static_cast<DROPFILES*>(GlobalLock(hGlobal));
    df->pFiles = sizeof(DROPFILES);
    df->fWide = TRUE;

    wchar_t* pDst = reinterpret_cast<wchar_t*>(reinterpret_cast<char*>(df) + sizeof(DROPFILES));
    std::copy(path.begin(), path.end(), pDst);
    pDst[path.size()] = L'\0';
    pDst[path.size() + 1] = L'\0';

    GlobalUnlock(hGlobal);

    if (!SetClipboardData(CF_HDROP, hGlobal)) {
        GlobalFree(hGlobal);
        CloseClipboard();
        return false;
    }

    CloseClipboard();
    return true;
}

std::string generate_tree(const fs::path& dir_path, const std::string& prefix = "") {
    std::vector<fs::directory_entry> items;
    std::error_code ec;

    for (const auto& entry : fs::directory_iterator(dir_path, fs::directory_options::skip_permission_denied, ec)) {
        items.push_back(entry);
    }

    if (ec) return prefix + "└── [无权限访问]\n";

    std::sort(items.begin(), items.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {
        if (a.is_directory() != b.is_directory()) {
            return a.is_directory() > b.is_directory();
        }
        return a.path().filename().wstring() < b.path().filename().wstring();
    });

    std::string tree_str;
    for (size_t i = 0; i < items.size(); ++i) {
        bool is_last = (i == items.size() - 1);
        std::string pointer = is_last ? "└── " : "├── ";

        tree_str += prefix + pointer + wstring_to_utf8(items[i].path().filename().wstring()) + "\n";

        if (items[i].is_directory()) {
            std::string extension = is_last ? "    " : "│   ";
            tree_str += generate_tree(items[i].path(), prefix + extension);
        }
    }
    return tree_str;
}

int main() {
    std::wstring target_dir = get_clipboard_path();

    if (target_dir.empty()) {
        return 1;
    }

    fs::path target(target_dir);
    if (!fs::is_directory(target)) {
        return 1;
    }

    std::string output_name = "directory_tree.txt";
    fs::path out_path = fs::temp_directory_path() / output_name;

    std::string content = wstring_to_utf8(target.filename().wstring()) + "\n" + generate_tree(target);

    std::ofstream out_file(out_path, std::ios::binary);
    if (!out_file) {
        return 1;
    }

    out_file << content;
    out_file.close();

    set_clipboard_file(out_path.wstring());

    return 0;
}