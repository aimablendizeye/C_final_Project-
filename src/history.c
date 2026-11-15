

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "history.h"

#define ID_HISTORY_EMPLOYEE 601
#define ID_HISTORY_MONTH    602
#define ID_HISTORY_FILTER   603
#define ID_HISTORY_ALL      604
#define ID_HISTORY_LIST     605

HWND historyWindow = NULL;
HWND historyEmployeeField;
HWND historyMonthField;
HWND historyList;

/* Save one history record */
void AddHistoryRecord(
    const char *event,
    const char *employeeID,
    const char *payMonth,
    const char *details)
{
    FILE *file = fopen("database/history.txt", "a");

    if (file == NULL)
        return;

    time_t now = time(NULL);
    struct tm *local = localtime(&now);

    char timestamp[30];

    if (local != NULL)
    {
        strftime(timestamp, sizeof(timestamp),
                 "%Y-%m-%d %H:%M:%S", local);
    }
    else
    {
        strcpy(timestamp, "Unknown time");
    }

    /* Keep separators out of the stored fields */
    fprintf(file, "%s | %s | %s | %s | %s\n",
        timestamp,
        event ? event : "",
        employeeID ? employeeID : "",
        payMonth ? payMonth : "",
        details ? details : "");

    fclose(file);
}

/* Load and filter history records */
void LoadHistory(const char *employeeFilter, const char *monthFilter)
{
    SendMessageA(historyList, LB_RESETCONTENT, 0, 0);

    FILE *file = fopen("database/history.txt", "r");

    if (file == NULL)
    {
        SendMessageA(historyList, LB_ADDSTRING, 0,
                     (LPARAM)"No history records yet.");
        return;
    }

    char line[700];
    int found = 0;

    while (fgets(line, sizeof(line), file))
    {
        char copy[700];
        strcpy(copy, line);

        char *timestamp = strtok(copy, "|");
        char *event = strtok(NULL, "|");
        char *id = strtok(NULL, "|");
        char *month = strtok(NULL, "|");

        if (timestamp == NULL || event == NULL ||
            id == NULL || month == NULL)
            continue;

        /* Remove spaces around fields */
        while (*id == ' ')
            id++;

        while (*month == ' ')
            month++;

        char *endID = id + strlen(id) - 1;
        while (endID >= id && (*endID == ' ' || *endID == '\n'))
            *endID-- = '\0';

        char *endMonth = month + strlen(month) - 1;
        while (endMonth >= month &&
               (*endMonth == ' ' || *endMonth == '\n'))
            *endMonth-- = '\0';

        if (employeeFilter[0] &&
            strcmp(employeeFilter, id) != 0)
            continue;

        if (monthFilter[0] &&
            strcmp(monthFilter, month) != 0)
            continue;

        SendMessageA(historyList, LB_ADDSTRING, 0, (LPARAM)line);
        found++;
    }

    fclose(file);

    if (found == 0)
    {
        SendMessageA(historyList, LB_ADDSTRING, 0,
                     (LPARAM)"No matching history records found.");
    }
}

/* Apply filters */
void FilterHistory(HWND hwnd)
{
    char employeeID[30] = "";
    char month[20] = "";

    GetWindowTextA(historyEmployeeField,
                   employeeID, sizeof(employeeID));

    GetWindowTextA(historyMonthField,
                   month, sizeof(month));

    LoadHistory(employeeID, month);
}

/* History window procedure */
LRESULT CALLBACK HistoryWindowProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_COMMAND:

            if (LOWORD(wParam) == ID_HISTORY_FILTER)
            {
                FilterHistory(hwnd);
                return 0;
            }

            if (LOWORD(wParam) == ID_HISTORY_ALL)
            {
                SetWindowTextA(historyEmployeeField, "");
                SetWindowTextA(historyMonthField, "");
                LoadHistory("", "");
                return 0;
            }

            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            historyWindow = NULL;
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* Open history window */
void ShowHistoryForm(HWND hwnd)
{
    if (historyWindow != NULL && IsWindow(historyWindow))
    {
        ShowWindow(historyWindow, SW_SHOW);
        SetForegroundWindow(historyWindow);
        LoadHistory("", "");
        return;
    }

    static int registered = 0;

    if (!registered)
    {
        WNDCLASSA wc = {0};

        wc.lpfnWndProc = HistoryWindowProc;
        wc.hInstance = GetModuleHandleA(NULL);
        wc.lpszClassName = "HistoryWindow";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor = LoadCursorA(NULL, IDC_ARROW);

        if (!RegisterClassA(&wc))
        {
            MessageBoxA(hwnd, "Could not create history window.",
                        "Error", MB_OK | MB_ICONERROR);
            return;
        }

        registered = 1;
    }

    historyWindow = CreateWindowExA(
        0, "HistoryWindow", "System Activity History",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        850, 550,
        hwnd, NULL, GetModuleHandleA(NULL), NULL
    );

    if (historyWindow == NULL)
        return;

    CreateWindowA("STATIC", "Employee ID:",
        WS_CHILD | WS_VISIBLE,
        20, 20, 100, 25,
        historyWindow, NULL, NULL, NULL);

    historyEmployeeField = CreateWindowExA(
        WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        120, 18, 150, 27,
        historyWindow, (HMENU)(INT_PTR)ID_HISTORY_EMPLOYEE,
        GetModuleHandleA(NULL), NULL);

    CreateWindowA("STATIC", "Pay Month:",
        WS_CHILD | WS_VISIBLE,
        290, 20, 80, 25,
        historyWindow, NULL, NULL, NULL);

    historyMonthField = CreateWindowExA(
        WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        370, 18, 120, 27,
        historyWindow, (HMENU)(INT_PTR)ID_HISTORY_MONTH,
        GetModuleHandleA(NULL), NULL);

    CreateWindowA("BUTTON", "Filter",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        510, 17, 90, 30,
        historyWindow, (HMENU)(INT_PTR)ID_HISTORY_FILTER,
        GetModuleHandleA(NULL), NULL);

    CreateWindowA("BUTTON", "Show All",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        610, 17, 90, 30,
        historyWindow, (HMENU)(INT_PTR)ID_HISTORY_ALL,
        GetModuleHandleA(NULL), NULL);

    historyList = CreateWindowExA(
        WS_EX_CLIENTEDGE, "LISTBOX", "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL |
        WS_HSCROLL | LBS_NOINTEGRALHEIGHT,
        20, 65, 790, 420,
        historyWindow, (HMENU)(INT_PTR)ID_HISTORY_LIST,
        GetModuleHandleA(NULL), NULL);

    SendMessageA(historyList, LB_SETHORIZONTALEXTENT,
                 1100, 0);

    LoadHistory("", "");
}