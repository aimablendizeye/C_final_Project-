
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "attendance.h"
#include "employee.h"
#include "history.h"

#define ID_ATT_EMPLOYEE 301
#define ID_ATT_MONTH    302
#define ID_ATT_WORKED   303
#define ID_ATT_ABSENT   304
#define ID_ATT_OVERTIME 305
#define ID_ATT_SAVE     306

HWND attendanceWindow = NULL;
HWND attendanceFields[5];

/* Create a label */
void AttendanceLabel(HWND hwnd, const char *text, int x, int y)
{
    CreateWindowA(
        "STATIC", text,
        WS_CHILD | WS_VISIBLE,
        x, y, 160, 25,
        hwnd, NULL, NULL, NULL
    );
}

/* Create an input field */
HWND AttendanceInput(HWND hwnd, int id, int x, int y)
{
    return CreateWindowExA(
        WS_EX_CLIENTEDGE,
        "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        x, y, 250, 27,
        hwnd, (HMENU)(INT_PTR)id,
        GetModuleHandleA(NULL), NULL
    );
}

/* Check that a value contains only digits */
int IsWholeNumber(const char *text)
{
    if (*text == '\0')
        return 0;

    while (*text)
    {
        if (!isdigit((unsigned char)*text))
            return 0;

        text++;
    }

    return 1;
}

/* Check month format MM-YYYY */
int IsValidPayMonth(const char *month)
{
    if (strlen(month) != 7)
        return 0;

    if (month[2] != '-')
        return 0;

    for (int i = 0; i < 7; i++)
    {
        if (i != 2 && !isdigit((unsigned char)month[i]))
            return 0;
    }

    int m = (month[0] - '0') * 10 + (month[1] - '0');
    int y = atoi(month + 3);

    if (m < 1 || m > 12 || y < 1)
        return 0;

    return 1;
}

/* Check whether the employee exists and is not terminated */
int CheckAttendanceEmployee(const char *employeeID)
{
    FILE *file = fopen("database/employees.dat", "rb");

    if (file == NULL)
        return 0;

    Employee employee;

    while (fread(&employee, sizeof(Employee), 1, file) == 1)
    {
        if (strcmp(employee.employeeID, employeeID) == 0)
        {
            fclose(file);

            if (strcmp(employee.status, "Terminated") == 0)
                return -1;

            return 1;
        }
    }

    fclose(file);
    return 0;
}

/* Check whether attendance already exists for this employee and month */
int AttendanceAlreadyExists(const char *employeeID, const char *month)
{
    FILE *file = fopen("database/attendance.dat", "rb");

    if (file == NULL)
        return 0;

    Attendance record;

    while (fread(&record, sizeof(Attendance), 1, file) == 1)
    {
        if (strcmp(record.employeeID, employeeID) == 0 &&
            strcmp(record.payMonth, month) == 0)
        {
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}

/* Save attendance information */
void SaveAttendance(HWND hwnd)
{
    Attendance record = {0};

    char workedText[30];
    char absentText[30];
    char overtimeText[30];

    GetWindowTextA(attendanceFields[0],
                   record.employeeID, sizeof(record.employeeID));

    GetWindowTextA(attendanceFields[1],
                   record.payMonth, sizeof(record.payMonth));

    GetWindowTextA(attendanceFields[2],
                   workedText, sizeof(workedText));

    GetWindowTextA(attendanceFields[3],
                   absentText, sizeof(absentText));

    GetWindowTextA(attendanceFields[4],
                   overtimeText, sizeof(overtimeText));

    /* Check empty fields */
    if (strlen(record.employeeID) == 0 ||
        strlen(record.payMonth) == 0 ||
        strlen(workedText) == 0 ||
        strlen(absentText) == 0 ||
        strlen(overtimeText) == 0)
    {
        MessageBoxA(hwnd,
            "Please complete all attendance fields.",
            "Missing Information",
            MB_OK | MB_ICONWARNING);
        return;
    }

    /* Check employee ID */
    int employeeResult =
        CheckAttendanceEmployee(record.employeeID);

    if (employeeResult == 0)
    {
        MessageBoxA(hwnd,
            "Employee ID was not found.",
            "Invalid Employee",
            MB_OK | MB_ICONWARNING);
        return;
    }

    if (employeeResult == -1)
    {
        MessageBoxA(hwnd,
            "Attendance cannot be entered for a terminated employee.",
            "Terminated Employee",
            MB_OK | MB_ICONWARNING);
        return;
    }

    /* Validate pay month */
    if (!IsValidPayMonth(record.payMonth))
    {
        MessageBoxA(hwnd,
            "Enter the pay month in MM-YYYY format.",
            "Invalid Month",
            MB_OK | MB_ICONWARNING);
        return;
    }

    /* Validate days worked */
    if (!IsWholeNumber(workedText))
    {
        MessageBoxA(hwnd, "Days worked must be a whole number.",
                    "Invalid Days Worked",
                    MB_OK | MB_ICONWARNING);
        return;
    }

    /* Validate days absent */
    if (!IsWholeNumber(absentText))
    {
        MessageBoxA(hwnd, "Days absent must be a whole number.",
                    "Invalid Days Absent",
                    MB_OK | MB_ICONWARNING);
        return;
    }

    record.daysWorked = atoi(workedText);
    record.daysAbsent = atoi(absentText);

    if (record.daysWorked < 0 ||
        record.daysAbsent < 0 ||
        record.daysWorked + record.daysAbsent > 22)
    {
        MessageBoxA(hwnd,
            "Days worked and absent must total no more than 22.",
            "Invalid Attendance",
            MB_OK | MB_ICONWARNING);
        return;
    }

    /* Validate overtime */
    char *endPointer;

    record.overtimeHours = strtod(overtimeText, &endPointer);

    if (endPointer == overtimeText ||
        *endPointer != '\0' ||
        record.overtimeHours < 0 ||
        record.overtimeHours > 60)
    {
        MessageBoxA(hwnd,
            "Overtime must be between 0 and 60 hours.",
            "Invalid Overtime",
            MB_OK | MB_ICONWARNING);
        return;
    }

    /* Prevent duplicate monthly attendance */
    if (AttendanceAlreadyExists(
            record.employeeID, record.payMonth))
    {
        MessageBoxA(hwnd,
            "Attendance has already been entered for this employee and month.",
            "Duplicate Attendance",
            MB_OK | MB_ICONWARNING);
        return;
    }

    /* Save attendance record */
    FILE *file = fopen("database/attendance.dat", "ab");

    if (file == NULL)
    {
        MessageBoxA(hwnd,
            "Could not open attendance database.",
            "File Error",
            MB_OK | MB_ICONERROR);
        return;
    }

    if (fwrite(&record, sizeof(Attendance), 1, file) != 1)
    {
        fclose(file);

        MessageBoxA(hwnd,
            "Attendance could not be saved.",
            "Save Error",
            MB_OK | MB_ICONERROR);
        return;
    }

    fclose(file);

/* Record this activity in history */
AddHistoryRecord(
    "Attendance Saved",
    record.employeeID,
    record.payMonth,
    "Attendance and overtime recorded"
);

MessageBoxA(hwnd,
    "Attendance saved successfully!",
    "Success",
    MB_OK | MB_ICONINFORMATION);

    for (int i = 0; i < 5; i++)
        SetWindowTextA(attendanceFields[i], "");

    SetFocus(attendanceFields[0]);
}

/* Attendance window procedure */
LRESULT CALLBACK AttendanceWindowProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_ATT_SAVE)
            {
                SaveAttendance(hwnd);
                return 0;
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            attendanceWindow = NULL;
            return 0;

        case WM_DESTROY:
            attendanceWindow = NULL;
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* Open attendance form */
void ShowAttendanceForm(HWND hwnd)
{
    if (attendanceWindow != NULL &&
        IsWindow(attendanceWindow))
    {
        ShowWindow(attendanceWindow, SW_SHOW);
        SetForegroundWindow(attendanceWindow);
        return;
    }

    static int classRegistered = 0;

    if (!classRegistered)
    {
        WNDCLASSA wc = {0};

        wc.lpfnWndProc = AttendanceWindowProc;
        wc.hInstance = GetModuleHandleA(NULL);
        wc.lpszClassName = "AttendanceWindow";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor = LoadCursorA(NULL, IDC_ARROW);

        if (!RegisterClassA(&wc))
        {
            MessageBoxA(hwnd,
                "Could not create attendance window.",
                "Error", MB_OK | MB_ICONERROR);
            return;
        }

        classRegistered = 1;
    }

    attendanceWindow = CreateWindowExA(
        0,
        "AttendanceWindow",
        "Employee Attendance and Overtime",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        550, 470,
        hwnd, NULL,
        GetModuleHandleA(NULL), NULL
    );

    if (attendanceWindow == NULL)
        return;

    AttendanceLabel(attendanceWindow, "Employee ID:", 30, 35);
    attendanceFields[0] =
        AttendanceInput(attendanceWindow, ID_ATT_EMPLOYEE, 200, 35);

    AttendanceLabel(attendanceWindow, "Pay Month (MM-YYYY):", 30, 95);
    attendanceFields[1] =
        AttendanceInput(attendanceWindow, ID_ATT_MONTH, 200, 95);

    AttendanceLabel(attendanceWindow, "Days Worked:", 30, 155);
    attendanceFields[2] =
        AttendanceInput(attendanceWindow, ID_ATT_WORKED, 200, 155);

    AttendanceLabel(attendanceWindow, "Days Absent:", 30, 215);
    attendanceFields[3] =
        AttendanceInput(attendanceWindow, ID_ATT_ABSENT, 200, 215);

    AttendanceLabel(attendanceWindow, "Overtime Hours:", 30, 275);
    attendanceFields[4] =
        AttendanceInput(attendanceWindow, ID_ATT_OVERTIME, 200, 275);

    CreateWindowA(
        "BUTTON", "Save Attendance",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        200, 345, 180, 40,
        attendanceWindow,
        (HMENU)(INT_PTR)ID_ATT_SAVE,
        GetModuleHandleA(NULL), NULL
    );
}
