
#include <windows.h>
#include "employee.h"

/* Navigation button IDs */
#define ID_DASHBOARD  101
#define ID_EMPLOYEES  102
#define ID_ATTENDANCE 103
#define ID_PAYROLL    104
#define ID_PAYSLIPS   105
#define ID_HISTORY    106
#define ID_REPORTS    107

/* Function to draw text */
void DrawLabel(HDC hdc, const char *text, int x, int y,
               int width, int height, HFONT font, COLORREF color)
{
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);

    HFONT oldFont = (HFONT)SelectObject(hdc, font);

    RECT rect = {x, y, x + width, y + height};
    DrawTextA(hdc, text, -1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
}

/* Function to draw dashboard cards */
void DrawCard(HDC hdc, int x, const char *title,
              const char *value, HFONT titleFont, HFONT valueFont)
{
    RECT card = {x, 150, x + 245, 270};

    HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdc, &card, brush);
    DeleteObject(brush);

    DrawLabel(hdc, title, x + 20, 165, 205, 30,
              titleFont, RGB(100, 110, 125));

    DrawLabel(hdc, value, x + 20, 205, 205, 45,
              valueFont, RGB(30, 45, 65));
}

/* Create the dashboard navigation buttons */
void CreateNavigationButtons(HWND hwnd)
{
    const char *labels[] = {
        "Dashboard",
        "Employees",
        "Attendance",
        "Payroll",
        "Payslips",
        "History",
        "Reports"
    };

    int ids[] = {
        ID_DASHBOARD,
        ID_EMPLOYEES,
        ID_ATTENDANCE,
        ID_PAYROLL,
        ID_PAYSLIPS,
        ID_HISTORY,
        ID_REPORTS
    };

    for (int i = 0; i < 7; i++)
    {
        CreateWindowExA(
            0,
            "BUTTON",
            labels[i],
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 150 + (i * 55),
            180, 42,
            hwnd,
            (HMENU)(INT_PTR)ids[i],
            GetModuleHandleA(NULL),
            NULL
        );
    }
}

/* Main window procedure */
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg,
                            WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            CreateNavigationButtons(hwnd);
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case ID_DASHBOARD:
                    MessageBoxA(hwnd, "You are on the Dashboard.",
                                "Dashboard", MB_OK);
                    break;

                case ID_EMPLOYEES:
                    ShowEmployeeForm(hwnd);
                    break;

                case ID_ATTENDANCE:
                    MessageBoxA(hwnd, "Attendance Management is coming soon.",
                                "Attendance", MB_OK);
                    break;

                case ID_PAYROLL:
                    MessageBoxA(hwnd, "Payroll Management is coming soon.",
                                "Payroll", MB_OK);
                    break;

                case ID_PAYSLIPS:
                    MessageBoxA(hwnd, "Payslip Management is coming soon.",
                                "Payslips", MB_OK);
                    break;

                case ID_HISTORY:
                    MessageBoxA(hwnd, "System History is coming soon.",
                                "History", MB_OK);
                    break;

                case ID_REPORTS:
                    MessageBoxA(hwnd, "Payroll Reports are coming soon.",
                                "Reports", MB_OK);
                    break;
            }
            return 0;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT client;
            GetClientRect(hwnd, &client);

            /* Main background */
            HBRUSH background = CreateSolidBrush(RGB(245, 247, 250));
            FillRect(hdc, &client, background);
            DeleteObject(background);

            /* Sidebar */
            RECT sidebar = {0, 0, 220, client.bottom};
            HBRUSH navy = CreateSolidBrush(RGB(25, 42, 70));
            FillRect(hdc, &sidebar, navy);
            DeleteObject(navy);

            /* Fonts */
            HFONT headingFont = CreateFontA(
                28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                DEFAULT_PITCH, "Segoe UI"
            );

            HFONT normalFont = CreateFontA(
                16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                DEFAULT_PITCH, "Segoe UI"
            );

            HFONT smallFont = CreateFontA(
                14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                DEFAULT_PITCH, "Segoe UI"
            );

            /* Sidebar heading */
            DrawLabel(hdc, "PAYROLL SYSTEM",
                      20, 35, 190, 35,
                      headingFont, RGB(255, 255, 255));

            DrawLabel(hdc, "EMPLOYEE MANAGEMENT",
                      20, 80, 190, 25,
                      smallFont, RGB(190, 205, 225));

            /* Main dashboard heading */
            DrawLabel(hdc, "Dashboard",
                      250, 35, 500, 45,
                      headingFont, RGB(30, 45, 65));

            DrawLabel(hdc, "Welcome to your Employee Payroll System",
                      250, 85, 600, 30,
                      normalFont, RGB(100, 110, 125));

            /* Summary cards */
            DrawCard(hdc, 250, "Total Employees",
                     "0", smallFont, headingFont);

            DrawCard(hdc, 520, "Monthly Payroll",
                     "0.00", smallFont, headingFont);

            DrawCard(hdc, 790, "Attendance",
                     "0%", smallFont, headingFont);

            /* Lower dashboard section */
            DrawLabel(hdc, "System Overview",
                      250, 320, 500, 35,
                      headingFont, RGB(30, 45, 65));

            DrawLabel(hdc, "Your payroll information will appear here.",
                      250, 375, 650, 30,
                      normalFont, RGB(100, 110, 125));

            DeleteObject(headingFont);
            DeleteObject(normalFont);
            DeleteObject(smallFont);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

/* Application entry point */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    const char CLASS_NAME[] = "PayrollWindow";

    WNDCLASSA wc = {0};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);

    if (!RegisterClassA(&wc))
        return 0;

    HWND hwnd = CreateWindowExA(
        0,
        CLASS_NAME,
        "Employee Payroll Management System",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1150, 750,
        NULL, NULL,
        hInstance, NULL
    );

    if (hwnd == NULL)
        return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {0};

    while (GetMessageA(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}
