
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "employee.h"
#include "attendance.h"
#include "payroll.h"
#include "payslip.h"
#include "history.h"
#include "report.h"

/* Navigation button IDs */
#define ID_DASHBOARD  101
#define ID_EMPLOYEES  102
#define ID_ATTENDANCE 103
#define ID_PAYROLL    104
#define ID_PAYSLIPS   105
#define ID_HISTORY    106
#define ID_REPORTS    107
#define ID_REFRESH_DASHBOARD 108


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
void DrawCard(
    HDC hdc,
    int x,
    int y,
    const char *title,
    const char *value,
    HFONT titleFont,
    HFONT valueFont)
{
    RECT card = {x, y, x + 245, y + 120};

    HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));

    FillRect(hdc, &card, brush);

    DeleteObject(brush);

    DrawLabel(hdc, title,
              x + 20, y + 15, 205, 30,
              titleFont, RGB(100, 110, 125));

    DrawLabel(hdc, value,
              x + 20, y + 55, 205, 45,
              valueFont, RGB(30, 45, 65));
}

/* Dashboard statistics */
char dashboardEmployees[30] = "0";
char dashboardGross[50] = "0.00";
char dashboardNet[50] = "0.00";
char dashboardAttendance[30] = "0%";
char dashboardDepartments[500] = "";

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


/* Calculate dashboard statistics from saved files */
void LoadDashboardData(
    char *employeeText,
    char *grossText,
    char *netText,
    char *attendanceText,
    char *departmentText)
{
    int activeEmployees = 0;
    int departmentCounts[5] = {0};

    double totalGross = 0;
    double totalNet = 0;

    int totalWorked = 0;
    int totalAbsent = 0;

    const char *departments[] = {
        "Finance", "IT", "Sales", "Operations", "HR"
    };

    /* Get current month */
    SYSTEMTIME currentTime;
    GetLocalTime(&currentTime);

    char month[10];

    sprintf(month, "%02d-%04d",
            currentTime.wMonth,
            currentTime.wYear);

    /* Read employee records */
    FILE *file = fopen("database/employees.dat", "rb");

    if (file != NULL)
    {
        Employee e;

        while (fread(&e, sizeof(Employee), 1, file) == 1)
        {
            if (strcmp(e.status, "Active") == 0)
            {
                activeEmployees++;

                for (int i = 0; i < 5; i++)
                {
                    if (strcmp(e.department, departments[i]) == 0)
                        departmentCounts[i]++;
                }
            }
        }

        fclose(file);
    }

    /* Read payroll records for current month */
    file = fopen("database/payroll.dat", "rb");

    if (file != NULL)
    {
        Payroll p;

        while (fread(&p, sizeof(Payroll), 1, file) == 1)
        {
            if (strcmp(p.payMonth, month) == 0)
            {
                totalGross += p.grossSalary;
                totalNet += p.netSalary;
            }
        }

        fclose(file);
    }

    /* Read attendance records for current month */
    file = fopen("database/attendance.dat", "rb");

    if (file != NULL)
    {
        Attendance a;

        while (fread(&a, sizeof(Attendance), 1, file) == 1)
        {
            if (strcmp(a.payMonth, month) == 0)
            {
                totalWorked += a.daysWorked;
                totalAbsent += a.daysAbsent;
            }
        }

        fclose(file);
    }

    /* Prepare dashboard card values */
    sprintf(employeeText, "%d", activeEmployees);

    sprintf(grossText, "%.2f", totalGross);

    sprintf(netText, "%.2f", totalNet);

    double attendanceRate = 0;

    if (totalWorked + totalAbsent > 0)
    {
        attendanceRate =
            (double)totalWorked /
            (totalWorked + totalAbsent) * 100;
    }

    sprintf(attendanceText, "%.1f%%", attendanceRate);

    /* Prepare department overview */
    sprintf(
        departmentText,
        "Finance: %d employees\n"
        "IT: %d employees\n"
        "Sales: %d employees\n"
        "Operations: %d employees\n"
        "HR: %d employees",
        departmentCounts[0],
        departmentCounts[1],
        departmentCounts[2],
        departmentCounts[3],
        departmentCounts[4]
    );
}

void LoadRecentActivities(char *activityText)
{
    char lines[5][500] = {0};
    int count = 0;

    FILE *file = fopen("database/history.txt", "r");

    if (file == NULL)
    {
        strcpy(activityText, "No activities recorded yet.");
        return;
    }

    char line[500];

    while (fgets(line, sizeof(line), file) != NULL)
    {
        line[strcspn(line, "\r\n")] = '\0';

        if (strlen(line) == 0)
            continue;

        if (count < 5)
        {
            strcpy(lines[count], line);
            count++;
        }
        else
        {
            for (int i = 0; i < 4; i++)
                strcpy(lines[i], lines[i + 1]);

            strcpy(lines[4], line);
        }
    }

    fclose(file);

    activityText[0] = '\0';

    if (count == 0)
    {
        strcpy(activityText, "No activities recorded yet.");
        return;
    }

    for (int i = count - 1; i >= 0; i--)
    {
        strcat(activityText, lines[i]);
        strcat(activityText, "\n");
    }
}

void RefreshDashboard(HWND hwnd)
{
    LoadDashboardData(
        dashboardEmployees,
        dashboardGross,
        dashboardNet,
        dashboardAttendance,
        dashboardDepartments
        
    );

   

    InvalidateRect(hwnd, NULL, TRUE);
}

 char dashboardActivities[2600] = "";



/* Main window procedure */
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg,
                            WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
       case WM_CREATE:

            CreateNavigationButtons(hwnd);

        CreateWindowA(
           "BUTTON",
           "Refresh Dashboard",
           WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
           900, 105, 140, 30,
           hwnd,
           (HMENU)ID_REFRESH_DASHBOARD,
            GetModuleHandle(NULL),
            NULL
            );

            LoadDashboardData(
            dashboardEmployees,
            dashboardGross,
            dashboardNet,
            dashboardAttendance,
            dashboardDepartments
    );

    LoadRecentActivities(dashboardActivities);

    return 0;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {

                case ID_REFRESH_DASHBOARD:
                     RefreshDashboard(hwnd);
                     break;
                case ID_DASHBOARD:
                    MessageBoxA(hwnd, "You are on the Dashboard.",
                                "Dashboard", MB_OK);
                    break;

                case ID_EMPLOYEES:
                    ShowEmployeeForm(hwnd);
                    break;

               case ID_ATTENDANCE:
                    ShowAttendanceForm(hwnd);
                    break;

              case ID_PAYROLL:
                   ShowPayrollForm(hwnd);
                   break;

              case ID_PAYSLIPS:
                   ShowPayslipForm(hwnd);
                   break;

             case ID_HISTORY:
                  ShowHistoryForm(hwnd);
                  break;

            case ID_REPORTS:
                 ShowReportForm(hwnd);
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
         DrawCard(hdc, 250, 150, "Active Employees",
         dashboardEmployees, smallFont, headingFont);

         DrawCard(hdc, 520, 150, "Gross Payroll",
         dashboardGross, smallFont, headingFont);

         DrawCard(hdc, 790, 150, "Attendance Rate",
         dashboardAttendance, smallFont, headingFont);

         DrawCard(hdc, 250, 290, "Net Salary Paid",
         dashboardNet, smallFont, headingFont);


         /* System Overview */
/* System Overview */
DrawLabel(
    hdc,
    "Employees by Department",
    250, 445, 350, 35,
    headingFont,
    RGB(30, 45, 65)
);

RECT overviewRect = {250, 485, 600, 650};

DrawLabel(
    hdc,
    "Recent System Activities",
    650, 445, 400, 35,
    headingFont,
    RGB(30, 45, 65)
);

RECT activityRect = {650, 485, 1100, 650};

SetTextColor(hdc, RGB(80, 90, 105));
SetBkMode(hdc, TRANSPARENT);

HFONT oldFont = (HFONT)SelectObject(hdc, smallFont);

DrawTextA(
    hdc,
    dashboardDepartments,
    -1,
    &overviewRect,
    DT_LEFT | DT_TOP
);

DrawTextA(
    hdc,
    dashboardActivities,
    -1,
    &activityRect,
    DT_LEFT | DT_TOP
);

SelectObject(hdc, oldFont);

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
    /* Create required storage folders if they do not exist */
     CreateDirectoryA("database", NULL);
     CreateDirectoryA("reports", NULL);
     CreateDirectoryA("payslips", NULL);
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
