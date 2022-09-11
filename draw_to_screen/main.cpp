#include "draw_to_screen.h"

/*
void drawImage(HDC hdc)
{
    Graphics graphics(hdc);
    // Create an Image object.
    Image image(L"C:\\ariel\\projects\\VeganVirus\\download.png");
    // Create a Pen object.
    Pen pen(Color(255, 255, 0, 0), 2);
    // Draw the original source image.
    graphics.DrawImage(&image, 100, 10);
}
*/
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

Draw* draw;
Bitmap* bmp;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{   
    MSG msg;
    const wchar_t CLASS_NAME[] = L"Static";
    WNDCLASS wc = {};
    wc.lpfnWndProc = &WindowProc; //attach this callback procedure
    wc.hInstance = hInstance; //handle to application instance
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc); //register wc
    // Create the window.

    draw = new Draw(hInstance);
    bmp = new Bitmap(L"draw_to_screen.ico");

    SetTimer(draw->_hwnd, 1, 1. / 30, NULL);
    
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    delete draw;
    delete bmp;
    return 0;

}

int i = 0;
//callback procedure for this window, takes in all the window details
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CLOSE:
        return TRUE;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_PAINT:
        i += 4;
        draw->clear();
        draw->drawImage(bmp, i % 1000, i / 1000 * 16);
        break;
    case WM_TIMER:
        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
        break;
    default:
        break;
    }


    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}