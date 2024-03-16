/*#########################################
Autorzy:2
Kamil Kowalewski 216806
Jakub Plich 216866
#########################################*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <limits>
#include <fstream>
#include <future>
#include <chrono>

#define SOH 0x1
#define EOT 0x4
#define ACK 0x6
#define NAK 0x15

#define C 'C'
#define SUB 26

using namespace std;

////////////////// ZMIENNE GLOBALNE SYSTEMOWE //////////////////////
HANDLE handleCom;
BOOL isReadyPort;
DCB controlDCB;
COMMTIMEOUTS objCommtime;

//JESLI BEDZIE NIE DZIALAC TO ODKOMNETOWAC
int BaudRate = 57600;
// int BaudRate = 9600;

bool crc;
char *COM;
const char *file;

//////////////////// FUNKCJE ///////////////////////
template<typename Type>
void isCorrect(Type &variable) {
    while ((!(cin >> variable))) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Wrong date, enter again: ";
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

//FUNKCJA ODPOWIEDZIALNA ZA INICJALIZACJE PORTU
void initialize(char *chosenPort) {
    handleCom = CreateFile(chosenPort, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    isReadyPort = SetupComm(handleCom, 1, 128); // USTAWIANIE WIELKOSCI BUFORA
    isReadyPort = GetCommState(handleCom, &controlDCB);
    controlDCB.BaudRate = BaudRate;
    controlDCB.ByteSize = 8;
    controlDCB.Parity = NOPARITY; //EVENPARITY;
    controlDCB.StopBits = ONESTOPBIT;
    controlDCB.fAbortOnError = TRUE;
    controlDCB.fOutX = FALSE; // XON/XOFF WYLACZANIE DO TRANSMISJI
    controlDCB.fInX = FALSE; // XON/XOFF WYLACZANIE DO ODBIERANIA
    controlDCB.fOutxCtsFlow = FALSE; // WLACZANIE CTS flow control
    controlDCB.fRtsControl = RTS_CONTROL_HANDSHAKE;
    controlDCB.fOutxDsrFlow = FALSE; //WLACZENIE DSR FLOW CONTROL
    controlDCB.fDtrControl = DTR_CONTROL_ENABLE;
    controlDCB.fDtrControl = DTR_CONTROL_DISABLE;
    controlDCB.fDtrControl = DTR_CONTROL_HANDSHAKE;
    isReadyPort = SetCommState(handleCom, &controlDCB);
}

//FUNKCJA ODPOWIEDZIALNA ZA ODCZYTYWANIE DANYCH Z PLIKU
void receiveCOM(char *sign, int length) {
    DWORD pos = 0, num;
    while (pos < length) {
        ReadFile(handleCom, sign + pos, length - pos, &num, NULL);
        pos += num;
    }
}


bool receiveCOM2(char *sign, int length) {
    DWORD pos = 0, num;
    while (pos < length) {
        ReadFile(handleCom, sign + pos, length - pos, &num, NULL);
        pos += num;
    }
    return true;
}


//FUNKCJA ODPOWIEDZIALNA ZA ZAPISYWANIE DANYCH DO PLIKU
void sendCOM(char *sign, int length) {
    DWORD num;
    WriteFile(handleCom, sign, length, &num, NULL);
}

//FUNKCJA ODPOWIEDZIALNA ZA CRC WERSJA 16 - WIELOMIAN 17 BITOWY
short int CRC16(char *fileBuffer) {
    int tmp = 0, val = 0x18005 << 15;
    for (int i = 0; i < 3; i++) {
        tmp = tmp * 256 + (unsigned char) fileBuffer[i];
    }
    tmp *= 256;

    for (int i = 3; i < 134; i++) {
        if (i < 128) {
            tmp += (unsigned char) fileBuffer[i];
        }
        for (int j = 0; j < 8; j++) {
            if (tmp & (1 << 31)) {
                tmp ^= val;
            }
            tmp <<= 1;
        }
    }
    return tmp >> 16;
}

//FUNKCJA ODPOWIEDZIALNA ZA ODBIERANIE DANYCH - KORZYSTA Z FUNKCJI sendCOM() oraz receiveCOM()
void receiving() {
    char buf[3], fileBuffer[128];
    //initialize(COM);

    buf[0] = crc ? C : NAK;             //WYSYLANIE ZNAKU NAK
    sendCOM(buf, 1);

    FILE *f = fopen(file, "wb"); //OTWIERANIE TRYB WB - TRYB BINARNY ORAZ NADPISYWANIE(KASUJE STARA ZAWARTOSC)


    auto timeout = std::chrono::seconds(1);

    while (true) {
        sendCOM(buf, 1);

        cout<<"znowu?"<<endl;
        //TODO pomysł jest na stworzenie osobnego wątku który będzie nadawał informację o tym że można odebrać informacje
        std::future<bool> resultFuture = std::async(std::launch::async, receiveCOM2, buf, 1);

        std::future_status status = resultFuture.wait_for(timeout);

        if (status == std::future_status::ready) {
            // Wynik jest dostępny w określonym czasie
            bool result = resultFuture.get();
            if (result) {
                break;
            }
        } else if (status == std::future_status::timeout) {
            cout<<"Tu bylen"<<endl;
            continue;
        }

    }


    // receiveCOM(buf, 1);                 //ODBIERANIE DANYCH
    printf("%d\n", buf[0]);
    while (true) {
        unsigned short sum, sumc;
        receiveCOM(buf + 1, 2);
        receiveCOM(fileBuffer, 128);

        sum = sumc = 0;
        receiveCOM((char *) &sum, crc ? 2 : 1);

        if (crc) {
            sumc = CRC16(fileBuffer);
        } else {
            for (int i = 0; i < 128; i++) {
                sumc += (unsigned char) fileBuffer[i];
            }
            sumc %= 256;
        }

        printf("%x %x %x\n", sum, sumc);
        if (sum != sumc) {
            buf[0] = NAK;
            sendCOM(buf, 1);
            continue;
        }

        buf[0] = ACK;
        sendCOM(buf, 1);

        receiveCOM(buf, 1);
        if (buf[0] == EOT) {
            unsigned char last = 127;
            while (fileBuffer[last] == SUB) {
                last--;
            }
            fwrite(fileBuffer, last + 1, 1, f);
            break;
        }
        fwrite(fileBuffer, 128, 1, f);

    }
    fclose(f);
    buf[0] = ACK;
    sendCOM(buf, 1);
}

//FUNKCJA ODPOWIEDZIALNA ZA ODBIERANIE DANYCH - KORZYSTA Z FUNKCJI sendCOM() oraz receivingCOM()
void sending() {
    char buf[3], fileBuffer[128];
    //initialize(COM);

    receiveCOM(buf, 1);
    printf("%d\n", buf[0]);
    if (buf[0] == NAK) {
        crc = false;
    } else if (buf[0] == C) {
        crc = true;
    } else {
        return;
    }

    int no = 1;


    //is.open ("C:\\Final.gif", ios::binary );
    // get length of file:
    // allocate memory:


    FILE *f = fopen(file, "rb");


    fseek(f, 0, SEEK_END);

    int fsize = ftell(f);


    fseek(f, 0, SEEK_SET);
    while (ftell(f) < fsize) {
        unsigned char length = fread(fileBuffer, 1, 128, f);
        for (int i = length; i < 128; i++) {
            fileBuffer[i] = SUB;
        }
        unsigned short sum = 0;

        sum = 0;

        if (crc) {
            sum = CRC16(fileBuffer);
        } else {
            for (int i = 0; i < 128; i++)
                sum += (unsigned char) fileBuffer[i];
            sum %= 256;
        }

        buf[0] = SOH;
        buf[1] = no;
        buf[2] = 255 - no;

        sendCOM(buf, 3);
        sendCOM(fileBuffer, 128);
        sendCOM((char *) &sum, crc ? 2 : 1);

        receiveCOM(buf, 1);
        if (buf[0] == ACK) {
            no++;
        } else {
            fseek(f, -128, SEEK_CUR);
        }
    }

    fclose(f);
    do {
        buf[0] = EOT;
        sendCOM(buf, 1);
        receiveCOM(buf, 1);
    } while (buf[0] != ACK);
}


std::string openFileDialog() {
    OPENFILENAME ofn;
    char szFileName[MAX_PATH] = "";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "All Files\0*.*\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = sizeof(szFileName);
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
        return std::string(ofn.lpstrFile);
    } else {
        return ""; // Pusty string w przypadku, gdy użytkownik anuluje wybór pliku
    }
}


int main() {
    std::string filePath;
    cout << "Autorzy:\nKamil Kowalewski 216806\nJakub Plich 216866\n";

    int choice;
    ///////////////// WYBOR TRYBU ////////////////////////
    do {
        ifstream in_file("a.txt", ios::binary);
        in_file.seekg(0, ios::end);
        int file_size = in_file.tellg();
        cout << "Size of the file is" << " " << file_size << " " << "bytes";
        in_file.seekg(0, ios::beg);
        file_size = in_file.tellg();
        cout << "Size of the file is" << " " << file_size << " " << "bytes";
        in_file.close();


        file = openFileDialog().c_str();
        filePath = file;
        if (!filePath.empty()) {
            std::cout << "Wybrany plik: " << filePath << std::endl;
        } else {
            std::cout << "Nie wybrano żadnego pliku." << std::endl;
        }


        cout << "Wybierz tryb pracy:"
                "\n1. Wysylanie"
                "\n2. Odbieranie"
                "\nWybor: ";
        isCorrect(choice);
    } while (!(choice == 1 || choice == 2));

    ////////////// WPISYWANIE NAZWY PLIKU /////////////////

    //file = "coded2.txt";

    ///////// WYBOR PORTU /////////////////////
    int portNumber;
    do {
        cout << "Wybierz port: \n"
                << "1. COM1\n"
                << "2. COM2\n"
                << "3. COM3\n"
                << "4. COM4\n"
                << "Wybor: ";

        isCorrect(portNumber);
    } while (!(portNumber == 1 || portNumber == 2 || portNumber == 3 || portNumber == 4));

    switch (portNumber) {
        case 1: {
            COM = "COM1";
            break;
        }
        case 2: {
            COM = "COM2";
            break;
        }
        case 3: {
            COM = "COM3";
            break;
        }
        case 4: {
            COM = "COM4";
            break;
        }
    }
    initialize(COM);
    //////////////WLACZANIE CRC /////////////////
    int CRCEnable;
    do {
        cout << "Wlaczyc CRC: "
                "\n1. Tak"
                "\n2. Nie"
                "\nWybor: ";
        isCorrect(CRCEnable);
    } while (!(CRCEnable == 1 || CRCEnable == 2));

    if (CRCEnable == 1) {
        crc = true;
    } else if (CRCEnable == 2) {
        crc = false;
    }

    ///////////// URUCHAMIANIE DZIALANIA /////////////
    switch (choice) {
        case 1: {
            sending();
            break;
        }
        case 2: {
            receiving();
            break;
        }
    }

    return 0;
}
