#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <string>

#pragma comment(lib, "winmm.lib")

enum EffectType {
    BLOOM,
    SHAKE
};

// Play bytebeat audio in background thread
void PlayBytebeatAudio() {
    const int SAMPLE_RATE = 8000;
    const int DURATION = 10; // 10 seconds per loop
    const int BUFFER_SIZE = SAMPLE_RATE * DURATION;

    uint8_t* buffer = new uint8_t[BUFFER_SIZE];

    // Generate creepy glitchy bytebeat
    for (int t = 0; t < BUFFER_SIZE; t++) {
        buffer[t] = (t * ((t >> 9 | t >> 13) & 25 & t >> 6)) ^ (t >> 4) * (t & (t >> 8));
    }

    // WAV header
    struct WAVHeader {
        char riff[4];
        uint32_t fileSize;
        char wave[4];
        char fmt[4];
        uint32_t fmtSize;
        uint16_t audioFormat;
        uint16_t channels;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample;
        char data[4];
        uint32_t dataSize;
    } header = {
        {'R','I','F','F'},
        36 + BUFFER_SIZE,
        {'W','A','V','E'},
        {'f','m','t',' '},
        16, 1, 1,
        SAMPLE_RATE,
        SAMPLE_RATE,
        1, 8,
        {'d','a','t','a'},
        BUFFER_SIZE
    };

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sizeof(header) + BUFFER_SIZE);
    if (hMem) {
        uint8_t* pMem = (uint8_t*)GlobalLock(hMem);
        if (pMem) {
            memcpy(pMem, &header, sizeof(header));
            memcpy(pMem + sizeof(header), buffer, BUFFER_SIZE);
            GlobalUnlock(hMem);

            PlaySoundA((LPCSTR)pMem, NULL, SND_MEMORY | SND_SYNC);
        }
        GlobalFree(hMem);
    }

    delete[] buffer;
}

bool Confirm(const char* message) {
    return MessageBoxA(NULL, message, "Confirmation", MB_YESNO | MB_ICONQUESTION) == IDYES;
}

void ShutdownComputer() {
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    // Get shutdown privilege
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
        tkp.PrivilegeCount = 1;
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
        CloseHandle(hToken);
    }

    // Shutdown immediately
    ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    srand((unsigned)time(NULL));

    // Two warnings
    if (!Confirm("WARNING: This will infect your computer! Continue?")) return 0;
    if (!Confirm("Last Warning!!! Computer will gagaga Continue?")) return 0;

    DWORD start = GetTickCount();
    EffectType effect = BLOOM;
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);

    // Start first bytebeat in background
    std::thread([]() { PlayBytebeatAudio(); }).detach();

    DWORD lastSoundTime = GetTickCount();
    const DWORD SHUTDOWN_TIME = 75000; // 75 seconds

    while (true) {
        DWORD elapsed = GetTickCount() - start;
        DWORD remaining = (SHUTDOWN_TIME - elapsed) / 1000;

        // Check if 75 seconds passed
        if (elapsed >= SHUTDOWN_TIME) {
            ShutdownComputer();
            break;
        }

        // ESC to exit early (without shutdown)
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break;

        HDC hdc = GetDC(NULL);
        effect = static_cast<EffectType>((elapsed / 10000) % 2);

        // Play bytebeat every 10 seconds
        if (GetTickCount() - lastSoundTime > 10000) {
            std::thread([]() { PlayBytebeatAudio(); }).detach();
            lastSoundTime = GetTickCount();
        }

        // Draw countdown in corner
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 0, 0));
        std::string countdown = "65zt " + std::to_string(remaining) + "s";
        TextOutA(hdc, 10, 10, countdown.c_str(), countdown.length());

        switch (effect) {
        case BLOOM: {
            for (int i = 0; i < 3; i++) {
                int dx = rand() % 6 - 3;
                int dy = rand() % 6 - 3;
                BitBlt(hdc, dx, dy, w, h, hdc, 0, 0, SRCCOPY);
            }
            if (rand() % 20 == 0) PatBlt(hdc, 0, 0, w, h, DSTINVERT);
            break;
        }
        case SHAKE: {
            int shakeX = rand() % 20 - 10;
            int shakeY = rand() % 20 - 10;
            BitBlt(hdc, shakeX, shakeY, w, h, hdc, 0, 0, SRCCOPY);
            if (rand() % 8 == 0)
                BitBlt(hdc, 0, 0, w, h, hdc, rand() % 30, rand() % 30, SRCINVERT);
            break;
        }
        }

        ReleaseDC(NULL, hdc);
        Sleep(20);
    }

    return 0;
}