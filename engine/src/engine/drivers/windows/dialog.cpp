#include "dialog.h"

#include "engine/core/string/string_utils.h"
#include "engine/drivers/windows/win32_prerequisites.h"

namespace my {

std::string OpenFileDialog(const std::vector<const char*>& p_filters) {
    std::string filter_str;
    if (p_filters.empty()) {
        filter_str = "*.*";
    } else {
        for (const auto& filter : p_filters) {
            filter_str.append(";*");
            filter_str.append(filter);
        }
        filter_str = filter_str.substr(1);
    }

    char buffer[1024] = { 0 };
    StringUtils::Sprintf(buffer, "Supported Files(%s)\n%s", filter_str.c_str(), filter_str.c_str());
    for (char* p = buffer; *p; ++p) {
        if (*p == '\n') {
            *p = '\0';
            break;
        }
    }

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    char szFile[260] = { 0 };
    ofn.lStructSize = sizeof(ofn);
    // ofn.hwndOwner = DEV_ASSERT(0);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = buffer;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn)) {
        return std::string(szFile);
    }

    return "";
}

bool OpenSaveDialog(std::filesystem::path& p_inout_path) {
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    char file_name[MAX_PATH]{ 0 };
    char extension[MAX_PATH]{ 0 };
    char dir[MAX_PATH]{ 0 };
    StringUtils::Strcpy(file_name, p_inout_path.filename().replace_extension().string());
    StringUtils::Strcpy(dir, p_inout_path.parent_path().string());
    StringUtils::Strcpy(extension, p_inout_path.extension().string());

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    // ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = extension + 1;
    ofn.lpstrInitialDir = dir;

    if (GetSaveFileNameA(&ofn)) {
        p_inout_path = std::filesystem::path(ofn.lpstrFile);
        return true;
    }

    return false;
}

}  // namespace my
