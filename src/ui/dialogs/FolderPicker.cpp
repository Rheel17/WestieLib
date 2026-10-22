#include "ui/dialogs/FolderPicker.h"

#include <windows.h>
#include <shobjidl.h>

#include "platform/NearestExistingFolder.h"
#include "platform/win32/UniqueHandle.h"
#include "platform/win32/Win32Error.h"

namespace westie::ui {

namespace {

void open_at(IFileOpenDialog& dialog, const std::filesystem::path& start) {
    // The dialog can only open at a folder that exists
    std::optional<std::filesystem::path> folder = platform::nearest_existing_folder(start);
    if (!folder) {
        return;
    }
    IShellItem* item = nullptr;
    platform::win32::check_hresult(SHCreateItemFromParsingName(folder->c_str(), nullptr, IID_PPV_ARGS(&item)), "SHCreateItemFromParsingName");
    platform::win32::UniqueComPtr<IShellItem> owned(item);
    platform::win32::check_hresult(dialog.SetFolder(owned.get()), "IFileDialog::SetFolder");
}

std::filesystem::path file_system_path(IShellItem& item) {
    PWSTR path = nullptr;
    platform::win32::check_hresult(item.GetDisplayName(SIGDN_FILESYSPATH, &path), "IShellItem::GetDisplayName");
    platform::win32::UniqueCoTaskString owned(path);
    return owned.get();
}

}

std::optional<std::filesystem::path> pick_folder(HWND owner, const wchar_t* title, const std::filesystem::path& start) {
    IFileOpenDialog* created = nullptr;
    platform::win32::check_hresult(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&created)), "CoCreateInstance");
    platform::win32::UniqueComPtr<IFileOpenDialog> dialog(created);

    FILEOPENDIALOGOPTIONS options = 0;
    platform::win32::check_hresult(dialog->GetOptions(&options), "IFileDialog::GetOptions");
    platform::win32::check_hresult(dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST), "IFileDialog::SetOptions");
    platform::win32::check_hresult(dialog->SetTitle(title), "IFileDialog::SetTitle");
    open_at(*dialog, start);

    HRESULT shown = dialog->Show(owner);
    if (shown == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        return std::nullopt;
    }
    platform::win32::check_hresult(shown, "IFileDialog::Show");

    IShellItem* result = nullptr;
    platform::win32::check_hresult(dialog->GetResult(&result), "IFileDialog::GetResult");
    platform::win32::UniqueComPtr<IShellItem> chosen(result);
    return file_system_path(*chosen);
}

}
