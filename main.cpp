#include <args.hxx>
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
            NULL,   // Description
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

    // Set the binary value (using empty string as value name for default value)
    result = RegSetValueExA(
        hKey,
        "",  // Default value
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

int main(int argc, char **argv)
{
    args::ArgumentParser parser("d2rreg is a simply CLI tool to set the registry values in Wine for launching Diablo 2 Resurrected instances via Token Authentication");
    //args::Group group(parser, "Arguments are exclusive", args::Group::Validators::Xor);
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<std::string> protectToken(parser, "protect-token", "Protects the token using CryptProtectData", { "protect-token" });
    args::ValueFlag<std::string> updateToken(parser, "update-token", "Protects the token and updates the registry in one go", {"update-token"});
    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help) {
        std::cout << parser;
        return 0;
    } catch (args::ParseError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    } catch (args::ValidationError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    std::vector<BYTE> encrypted;
    std::string token;

    if (protectToken) {
        token = args::get(protectToken);
    } else if (updateToken) {
        token = args::get(updateToken);
    } else {
        // no arguments provided - display help
        std::cout << parser;
        return 0;
    }

    if (token.size() < 1) {
        std::cout << parser;
        std::cerr << "d2rreg: no token provided!";
        return 1;
    }

    //std::cerr << "token: " << token << std::endl;

    if (!encryptString(token, encrypted)) {
        std::cerr << "Encryption failed!" << std::endl;
        return 1;
    }

    if (protectToken) {
        std::cout.write(reinterpret_cast<const char*>(encrypted.data()), encrypted.size());
        return 0;
    }

    if (updateRegistry(encrypted)) {
        std::cerr << "Registry updated successfully (" <<encrypted.size() << " bytes)!" << std::endl;
        //std::cerr << "Size: " << encrypted.size() << " bytes" << std::endl;
        return 0;
    }

    std::cerr << "Couldn't update registry!" << std::endl;
    return 1;

}
