#include <windows.h>
#include <wincrypt.h>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#pragma comment(lib, "crypt32.lib")

using namespace std;

bool encryptString(const std::string& input, std::vector<BYTE>& output) {
    // Define the entropy
    BYTE entropy[] = { 0xC8, 0x76, 0xF4, 0xAE, 0x4C, 0x95, 0x2E, 0xFE, 0xF2, 0xFA, 0x0F, 0x54, 0x19, 0xC0, 0x9C, 0x43 };

    DATA_BLOB entropyBlob;
    entropyBlob.pbData = entropy;
    entropyBlob.cbData = sizeof(entropy);

    // Prepare input data
    DATA_BLOB inputBlob;
    inputBlob.pbData = (BYTE*)input.c_str();
    inputBlob.cbData = (DWORD)(input.length() + 1); // Include null terminator

    DATA_BLOB outputBlob;

    // Encrypt the data
    if (!CryptProtectData(
            &inputBlob,
            NULL,                  // Description
            &entropyBlob,          // Optional entropy
            NULL,                  // Reserved
            NULL,                  // No prompt struct
            0,                     // Flags
            &outputBlob)
    ) {
        std::cerr << "Error: CryptProtectData failed. Error code: " << GetLastError() << std::endl;
        return false;
    }

    // Copy encrypted data to output vector
    output.assign(outputBlob.pbData, outputBlob.pbData + outputBlob.cbData);

    // Free the output blob
    LocalFree(outputBlob.pbData);

    return true;
}

bool updateRegistry(const std::vector<BYTE>& data) {
    HKEY hKey;
    LONG result;

    // Open or create the registry key
    result = RegCreateKeyExA(
        HKEY_CURRENT_USER,
        "SOFTWARE\\Blizzard Entertainment\\Battle.net\\Launch Options\\OSI",
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        NULL,
        &hKey,
        NULL
    );

    if (result != ERROR_SUCCESS) {
        std::cerr << "Error: Failed to open/create registry key. Error code: " << result << std::endl;
        return false;
    }

    // Set the WEB_TOKEN value to the protected token
    result = RegSetValueExA(
        hKey,
        "WEB_TOKEN",
        0,
        REG_BINARY,
        data.data(),
        (DWORD)data.size()
    );

    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS) {
        std::cerr << "Error: Failed to set registry value. Error code: " << result << std::endl;
        return false;
    }

    return true;
}

struct RenameWindowParams {
    std::string targetTitle;
    std::string newTitle;
    int renamed = 0;
};

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    // see: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enumwindows
    RenameWindowParams* params = reinterpret_cast<RenameWindowParams*>(lParam);
    char windowTitle[256];

    if (GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle))) {
        if (params->targetTitle == windowTitle && SetWindowTextA(hwnd, params->newTitle.c_str())) {
            ++params->renamed;
        }
    }
    return TRUE;
}

int renameWindow(const std::string& targetTitle, const std::string& newTitle) {
    RenameWindowParams params;
    params.targetTitle = targetTitle;
    params.newTitle = newTitle;

    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&params));
    return params.renamed;
}

// How often --watch-window looks at the window, in milliseconds.
const DWORD WatchInterval = 500;

bool parseInt(const char* text, int& value) {
    char* end = nullptr;
    const long parsed = std::strtol(text, &end, 10);
    if (end == text || *end != '\0') {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

// Fullscreen and borderless windows have no caption and sit where their
// monitor is, and a minimized window reports a position far off screen.
bool isWindowed(HWND hwnd) {
    return (GetWindowLongPtrA(hwnd, GWL_STYLE) & WS_CAPTION) == WS_CAPTION && !IsIconic(hwnd);
}

int moveWindow(const std::string& title, int x, int y) {
    HWND hwnd = FindWindowA(NULL, title.c_str());
    if (!hwnd) {
        std::cerr << "d2rreg: no '" << title << "' window found!" << std::endl;
        return 1;
    }
    if (!isWindowed(hwnd)) {
        std::cout << "Left the window in place, it is not in windowed mode" << std::endl;
        return 0;
    }

    RECT rect;
    GetWindowRect(hwnd, &rect);
    OffsetRect(&rect, x - rect.left, y - rect.top);
    if (!MonitorFromRect(&rect, MONITOR_DEFAULTTONULL)) {
        std::cout << "Left the window in place, " << x << "," << y << " is on no monitor" << std::endl;
        return 0;
    }

    SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    std::cout << "Moved the window to " << x << "," << y << std::endl;
    return 0;
}

int watchWindow(const std::string& title) {
    HWND hwnd = FindWindowA(NULL, title.c_str());
    if (!hwnd) {
        std::cerr << "d2rreg: no '" << title << "' window found!" << std::endl;
        return 1;
    }

    // A position is only reported once it held for an interval, so dragging
    // the window reports where it was dropped rather than every step.
    RECT previous = {};
    std::optional<POINT> reported;
    RECT rect;
    while (GetWindowRect(hwnd, &rect)) {
        const bool settled = rect.left == previous.left && rect.top == previous.top;
        const bool moved = !reported || rect.left != reported->x || rect.top != reported->y;
        if (settled && moved && isWindowed(hwnd)) {
            std::cout << rect.left << " " << rect.top << std::endl;
            reported = POINT{rect.left, rect.top};
        }
        previous = rect;
        Sleep(WatchInterval);
    }
    return 0;
}

void show_help(const char* app_name) {
    std::cout << "d2rreg is a simple CLI tool to set the registry values in Wine for launching Diablo 2 Resurrected instances via Token Authentication" << std::endl;
    std::cout << "Usage: " << app_name << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help            Display this help menu" << std::endl;
    std::cout << "  --protect-token <token> Protects the token using CryptProtectData" << std::endl;
    std::cout << "  --update-token <token>  Protects the token and updates the registry in one go" << std::endl;
    std::cout << "  --rename-window <title> Finds all 'Diablo II: Resurrected' windows and renames them to <title>." << std::endl;
    std::cout << "                          Exits with 1 if no such window exists (yet)." << std::endl;
    std::cout << "  --move-window <title> <x> <y>" << std::endl;
    std::cout << "                          Moves the window titled <title> to <x>,<y>, unless it is not in" << std::endl;
    std::cout << "                          windowed mode or would end up on no monitor." << std::endl;
    std::cout << "                          Exits with 1 if no such window exists." << std::endl;
    std::cout << "  --watch-window <title>  Prints '<x> <y>' whenever the window titled <title> is moved to a" << std::endl;
    std::cout << "                          new position in windowed mode, and exits once the window is gone." << std::endl;
    std::cout << "                          Exits with 1 if no such window exists." << std::endl;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        show_help(argv[0]);
        return 0;
    }

    std::string token;
    std::string mode;
    std::string windowRenameTitle;
    std::string windowTitle;
    int windowX = 0;
    int windowY = 0;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            show_help(argv[0]);
            return 0;
        } else if (arg == "--protect-token") {
            if (!mode.empty()) {
                 std::cerr << "Error: Only one of --protect-token, --update-token, --rename-window, --move-window or --watch-window can be used." << std::endl;
                 return 1;
            }
            if (i + 1 < argc) {
                token = argv[++i];
                mode = "protect";
            } else {
                std::cerr << "Error: --protect-token requires an argument." << std::endl;
                return 1;
            }
        } else if (arg == "--update-token") {
            if (!mode.empty()) {
                 std::cerr << "Error: Only one of --protect-token, --update-token, --rename-window, --move-window or --watch-window can be used." << std::endl;
                 return 1;
            }
            if (i + 1 < argc) {
                token = argv[++i];
                mode = "update";
            } else {
                std::cerr << "Error: --update-token requires an argument." << std::endl;
                return 1;
            }
        } else if (arg == "--rename-window") {
            if (!mode.empty()) {
                 std::cerr << "Error: Only one of --protect-token, --update-token, --rename-window, --move-window or --watch-window can be used." << std::endl;
                 return 1;
            }
            if (i + 1 < argc) {
                windowRenameTitle = argv[++i];
                mode = "rename";
            } else {
                std::cerr << "Error: --rename-window requires an argument." << std::endl;
                return 1;
            }
        } else if (arg == "--move-window") {
            if (!mode.empty()) {
                 std::cerr << "Error: Only one of --protect-token, --update-token, --rename-window, --move-window or --watch-window can be used." << std::endl;
                 return 1;
            }
            if (i + 3 < argc && parseInt(argv[i + 2], windowX) && parseInt(argv[i + 3], windowY)) {
                windowTitle = argv[i + 1];
                i += 3;
                mode = "move";
            } else {
                std::cerr << "Error: --move-window requires a title and two whole numbers." << std::endl;
                return 1;
            }
        } else if (arg == "--watch-window") {
            if (!mode.empty()) {
                 std::cerr << "Error: Only one of --protect-token, --update-token, --rename-window, --move-window or --watch-window can be used." << std::endl;
                 return 1;
            }
            if (i + 1 < argc) {
                windowTitle = argv[++i];
                mode = "watch";
            } else {
                std::cerr << "Error: --watch-window requires an argument." << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Error: Unknown argument " << arg << std::endl;
            show_help(argv[0]);
            return 1;
        }
    }

    if (mode.empty()) {
        show_help(argv[0]);
        return 0;
    }

    if (mode == "protect") {
        if (token.size() < 1) {
            std::cerr << "d2rreg: no token provided for --protect-token!" << std::endl;
            return 1;
        }
        std::vector<BYTE> encrypted;
        if (!encryptString(token, encrypted)) {
            std::cerr << "Encryption failed!" << std::endl;
            return 1;
        }
        std::cout.write(reinterpret_cast<const char*>(encrypted.data()), encrypted.size());
        return 0;
    }

    if (mode == "update") {
        if (token.size() < 1) {
            std::cerr << "d2rreg: no token provided for --update-token!" << std::endl;
            return 1;
        }
        std::vector<BYTE> encrypted;
        if (!encryptString(token, encrypted)) {
            std::cerr << "Encryption failed!" << std::endl;
            return 1;
        }
        if (updateRegistry(encrypted)) {
            std::cerr << "Registry updated successfully (" << encrypted.size() << " bytes)!" << std::endl;
            return 0;
        } else {
            std::cerr << "Couldn\'t update registry!" << std::endl;
            return 1;
        }
    }

    if (mode == "rename") {
        if (windowRenameTitle.empty()) {
            std::cerr << "d2rreg: no title provided for --rename-window!" << std::endl;
            return 1;
        }
        const int renamed = renameWindow("Diablo II: Resurrected", windowRenameTitle);
        if (renamed == 0) {
            std::cerr << "d2rreg: no 'Diablo II: Resurrected' window found!" << std::endl;
            return 1;
        }
        std::cout << "Renamed " << renamed << " window(s) to \"" << windowRenameTitle << "\"" << std::endl;
        return 0;
    }

    // Per monitor aware, so positions are physical pixels on every monitor
    // whatever its scaling, as they are for the game itself.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    if (mode == "move") {
        return moveWindow(windowTitle, windowX, windowY);
    }

    if (mode == "watch") {
        return watchWindow(windowTitle);
    }

    return 1;
}
