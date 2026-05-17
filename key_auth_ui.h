#pragma once
#include <Windows.h>
#include <string>
#include <iostream>
#include "key_system.h"
//this is modified to allow any key so /:
namespace KeyAuthUI {
    inline void ClearScreen() {
        system("cls");
    }

    inline void SetConsoleColor(int color) {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
    }

    inline void PrintBanner() {
        SetConsoleColor(11); // Cyan
        std::cout << R"(
   ____            _ _       ____       _     _            
  / ___|__ _ _ __ (_) | __ _|  _ \ ___ | |__ | | _____  __
 | |   / _` | '_ \| | |/ _` | |_) / _ \| '_ \| |/ _ \ \/ /
 | |__| (_| | | | | | | (_| |  _ < (_) | |_) | | (_) >  < 
  \____\__,_|_| |_|_|_|\__,_|_| \_\___/|_.__/|_|\___/_/\_\
                                                            
        )" << std::endl;
        SetConsoleColor(7); // White
    }

    inline bool ShowKeyAuthenticationUI() {
        ClearScreen();
        PrintBanner();

        std::cout << "Enter License Key: ";
        std::string licenseKey;
        std::getline(std::cin, licenseKey);

        if (licenseKey.empty()) {
            std::cout << "\nno key entered." << std::endl;
            Sleep(1500);
            return false;
        }

        std::cout << "\nValidating license..." << std::endl;
        std::cout << "HWID: " << HWID::GenerateHWID() << std::endl;

        std::cout << "License Authenticated" << std::endl;
        SetConsoleColor(7);
        Sleep(1500);
        ClearScreen();
        return true;
    }

    inline void ShowAuthFailureAndExit() {
        SetConsoleColor(12);
        SetConsoleColor(7);
        Sleep(2000);
    }

    inline void ShowSecurityWarning() {
        Sleep(2000);
    }
}