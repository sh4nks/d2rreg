#include <windows.h>
#include <wincrypt.h>
#include <iostream>
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

void show_help(const char* app_name) {
    std::cout << "d2rreg is a simple CLI tool to set the registry values in Wine for launching Diablo 2 Resurrected instances via Token Authentication" << std::endl;
    std::cout << "Usage: " << app_name << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help            Display this help menu" << std::endl;
    std::cout << "  --protect-token <token> Protects the token using CryptProtectData" << std::endl;
    std::cout << "  --update-token <token>  Protects the token and updates the registry in one go" << std::endl;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        show_help(argv[0]);
        return 0;
    }

    std::string token;
    std::string mode;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            show_help(argv[0]);
            return 0;
        } else if (arg == "--protect-token") {
            if (!mode.empty()) {
                 std::cerr << "Error: Only one of --protect-token or --update-token can be used." << std::endl;
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
                 std::cerr << "Error: Only one of --protect-token or --update-token can be used." << std::endl;
                 return 1;
            }
            if (i + 1 < argc) {
                token = argv[++i];
                mode = "update";
            } else {
                std::cerr << "Error: --update-token requires an argument." << std::endl;
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

    if (token.size() < 1) {
        std::cerr << "d2rreg: no token provided!" << std::endl;
        return 1;
    }

    std::vector<BYTE> encrypted;
    if (!encryptString(token, encrypted)) {
        std::cerr << "Encryption failed!" << std::endl;
        return 1;
    }

    if (mode == "protect") {
        std::cout.write(reinterpret_cast<const char*>(encrypted.data()), encrypted.size());
        return 0;
    }

    if (mode == "update") {
        if (updateRegistry(encrypted)) {
            std::cerr << "Registry updated successfully (" << encrypted.size() << " bytes)!" << std::endl;
            return 0;
        } else {
            std::cerr << "Couldn't update registry!" << std::endl;
            return 1;
        }
    }

    return 1;
}
