

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "employee.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "employee.h"
#include <ctype.h>
#include <commctrl.h>
#include "history.h"

/* IDs for form fields */
#define ID_VIEW_EMPLOYEES 212

HWND listWindow = NULL;
HWND employeeTable = NULL;
HWND minimumWageInput = NULL;

HWND updateWindow = NULL;
HWND updateFields[5];

char updateEmployeeID[20];

double minimumWage = 0.0;

#define ID_MIN_WAGE_INPUT 213
#define ID_APPLY_MIN_WAGE 214
#define ID_EMPLOYEE_ID  201
#define ID_FULL_NAME    202
#define ID_NATIONAL_ID  203
#define ID_DEPARTMENT   204
#define ID_JOB_TITLE    205
#define ID_EMPLOYMENT   206
#define ID_JOIN_DATE    207
#define ID_BANK_ACCOUNT 208
#define ID_SALARY       209
#define ID_REGISTER     210
#define ID_TERMINATE_EMPLOYEE 215
#define ID_UPDATE_EMPLOYEE 216
#define ID_SAVE_UPDATE 217

#define ID_UPDATE_DEPARTMENT 218
#define ID_UPDATE_JOB 219
#define ID_UPDATE_TYPE 220
#define ID_UPDATE_SALARY 221
#define ID_UPDATE_STATUS 222
/* Store the form controls */
HWND fields[10];

HWND formWindow;

/* Check whether a field contains only spaces */
int IsBlank(const char *text)
{
    while (*text)
    {
        if (!isspace((unsigned char)*text))
            return 0;

        text++;
    }

    return 1;
}

/* Check date format, calendar validity and future dates */
int IsValidDate(const char *date)
{
    int year, month, day;
    int position = 0;

    if (sscanf(date, "%4d-%2d-%2d%n",
               &year, &month, &day, &position) != 3)
        return 0;

    if (position != 10 || date[position] != '\0')
        return 0;

    if (year < 1 || month < 1 || month > 12)
        return 0;

    int leapYear = (year % 400 == 0) ||
                   (year % 4 == 0 && year % 100 != 0);

    int daysInMonth[] = {
        31, leapYear ? 29 : 28, 31, 30,
        31, 30, 31, 31, 30, 31, 30, 31
    };

    if (day < 1 || day > daysInMonth[month - 1])
        return 0;

    SYSTEMTIME today;
    GetLocalTime(&today);

    if (year > today.wYear ||
        (year == today.wYear && month > today.wMonth) ||
        (year == today.wYear && month == today.wMonth &&
         day > today.wDay))
        return 0;

    return 1;
}

/* Check for duplicate IDs and the employee limit */
int CheckExistingEmployees(const Employee *newEmployee)
{
    FILE *file = fopen("database/employees.dat", "rb");

    if (file == NULL)
        return 0;

    Employee oldEmployee;
    int count = 0;

    while (fread(&oldEmployee, sizeof(Employee), 1, file) == 1)
    {
        count++;

        if (strcmp(oldEmployee.employeeID,
                   newEmployee->employeeID) == 0)
        {
            fclose(file);
            return 1;
        }

        if (strcmp(oldEmployee.nationalID,
                   newEmployee->nationalID) == 0)
        {
            fclose(file);
            return 2;
        }
    }

    fclose(file);

    if (count >= MAX_EMPLOYEES)
        return 3;

    return 0;
}

LRESULT CALLBACK EmployeeWindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
);

/* Create a text label */
void CreateLabel(HWND hwnd, const char *text,
                 int x, int y, int width)
{
    CreateWindowA(
        "STATIC", text,
        WS_CHILD | WS_VISIBLE,
        x, y, width, 25,
        hwnd, NULL, NULL, NULL
    );
}

/* Create a text input */
HWND CreateInput(HWND hwnd, int id, int x, int y, int width)
{
    return CreateWindowExA(
        WS_EX_CLIENTEDGE,
        "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        x, y, width, 27,
        hwnd, (HMENU)(INT_PTR)id,
        NULL, NULL
    );
}

/* Create a dropdown */
HWND CreateDropdown(HWND hwnd, int id, int x, int y,
                    int width, const char *items[])
{
    HWND combo = CreateWindowA(
        "COMBOBOX", "",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        x, y, width, 150,
        hwnd, (HMENU)(INT_PTR)id,
        NULL, NULL
    );

    for (int i = 0; items[i] != NULL; i++)
    {
        SendMessageA(combo, CB_ADDSTRING, 0,
                     (LPARAM)items[i]);
    }

    SendMessageA(combo, CB_SETCURSEL, 0, 0);

    return combo;
}



/* Display the employee registration form */

LRESULT CALLBACK EmployeeWindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (uMsg)
    {
        case WM_COMMAND:

            switch (LOWORD(wParam))
            {
                case ID_REGISTER:
                    RegisterEmployee(hwnd);
                    return 0;

                case 211:
                    DestroyWindow(hwnd);
                    formWindow = NULL;
                    return 0;

                case ID_VIEW_EMPLOYEES:
                    ShowEmployeeList(hwnd);
                    return 0;
            }

            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            formWindow = NULL;
            return 0;

        case WM_DESTROY:
            formWindow = NULL;
            return 0;
    }

    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}



void ShowEmployeeForm(HWND hwnd)
{
    if (formWindow != NULL && IsWindow(formWindow))
    {
        ShowWindow(formWindow, SW_SHOW);
        SetForegroundWindow(formWindow);
        return;
    }

    static int classRegistered = 0;

    if (!classRegistered)
    {
        WNDCLASSA wc = {0};

        wc.lpfnWndProc = EmployeeWindowProc;
        wc.hInstance = GetModuleHandleA(NULL);
        wc.lpszClassName = "EmployeeFormWindow";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor = LoadCursorA(NULL, IDC_ARROW);

        if (!RegisterClassA(&wc))
        {
            MessageBoxA(hwnd, "Could not register employee window.",
                        "Error", MB_OK | MB_ICONERROR);
            return;
        }

        classRegistered = 1;
    }

    formWindow = CreateWindowExA(
        0,
        "EmployeeFormWindow",
        "Register New Employee",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        760, 650,
        hwnd, NULL,
        GetModuleHandleA(NULL), NULL
    );

    if (formWindow == NULL)
    {
        MessageBoxA(hwnd, "Could not open registration form.",
                    "Error", MB_OK | MB_ICONERROR);
        return;
    }

    CreateLabel(formWindow, "Employee ID:", 30, 30, 150);
    fields[0] = CreateInput(formWindow, ID_EMPLOYEE_ID,
                            200, 30, 450);

    CreateLabel(formWindow, "Full Name:", 30, 85, 150);
    fields[1] = CreateInput(formWindow, ID_FULL_NAME,
                            200, 85, 450);

    CreateLabel(formWindow, "National ID:", 30, 140, 150);
    fields[2] = CreateInput(formWindow, ID_NATIONAL_ID,
                            200, 140, 450);

    const char *departments[] = {
        "Finance", "IT", "Sales",
        "Operations", "HR", NULL
    };

    CreateLabel(formWindow, "Department:", 30, 195, 150);
    fields[3] = CreateDropdown(formWindow, ID_DEPARTMENT,
                               200, 195, 450, departments);

    CreateLabel(formWindow, "Job Title:", 30, 250, 150);
    fields[4] = CreateInput(formWindow, ID_JOB_TITLE,
                            200, 250, 450);

    const char *types[] = {
        "Permanent", "Contract", "Intern", NULL
    };

    CreateLabel(formWindow, "Employment Type:", 30, 305, 150);
    fields[5] = CreateDropdown(formWindow, ID_EMPLOYMENT,
                               200, 305, 450, types);

    CreateLabel(formWindow, "Joining Date (YYYY-MM-DD):",
                30, 360, 170);
    fields[6] = CreateInput(formWindow, ID_JOIN_DATE,
                            200, 360, 450);

    CreateLabel(formWindow, "Bank Account:", 30, 415, 150);
    fields[7] = CreateInput(formWindow, ID_BANK_ACCOUNT,
                            200, 415, 450);

    CreateLabel(formWindow, "Basic Salary:", 30, 470, 150);
    fields[8] = CreateInput(formWindow, ID_SALARY,
                            200, 470, 450);

    CreateWindowA(
        "BUTTON", "Register Employee",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        200, 530, 200, 40,
        formWindow, (HMENU)(INT_PTR)ID_REGISTER,
        GetModuleHandleA(NULL), NULL
    );

    CreateWindowA(
        "BUTTON", "Close",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        420, 530, 120, 40,
        formWindow, (HMENU)(INT_PTR)211,
        GetModuleHandleA(NULL), NULL
    );

    CreateWindowA("BUTTON", "View Employees",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        200, 580, 200, 35,
        formWindow, (HMENU)(INT_PTR)ID_VIEW_EMPLOYEES,
        GetModuleHandleA(NULL), NULL);
}



/* Read and check the registration form */
void RegisterEmployee(HWND hwnd)
{
    Employee employee = {0};

    char salaryText[50];
    char *endPointer;
    double salary;

    /* Read information from the form */
    GetWindowTextA(fields[0], employee.employeeID,
                   sizeof(employee.employeeID));

    GetWindowTextA(fields[1], employee.fullName,
                   sizeof(employee.fullName));

    GetWindowTextA(fields[2], employee.nationalID,
                   sizeof(employee.nationalID));

    GetWindowTextA(fields[3], employee.department,
                   sizeof(employee.department));

    GetWindowTextA(fields[4], employee.jobTitle,
                   sizeof(employee.jobTitle));

    GetWindowTextA(fields[5], employee.employmentType,
                   sizeof(employee.employmentType));

    GetWindowTextA(fields[6], employee.joiningDate,
                   sizeof(employee.joiningDate));

    GetWindowTextA(fields[7], employee.bankAccount,
                   sizeof(employee.bankAccount));

    GetWindowTextA(fields[8], salaryText,
                   sizeof(salaryText));

    /* Check that all fields are filled */
    if (IsBlank(employee.employeeID) ||
        IsBlank(employee.fullName) ||
        IsBlank(employee.nationalID) ||
        IsBlank(employee.department) ||
        IsBlank(employee.jobTitle) ||
        IsBlank(employee.employmentType) ||
        IsBlank(employee.joiningDate) ||
        IsBlank(employee.bankAccount) ||
        IsBlank(salaryText))
    {
        MessageBoxA(hwnd,
            "Please complete all employee information.",
            "Missing Information",
            MB_OK | MB_ICONWARNING);

        return;
    }

    /* Validate salary */
    salary = strtod(salaryText, &endPointer);

    if (endPointer == salaryText ||
        *endPointer != '\0' ||
        salary <= 0)
    {
        MessageBoxA(hwnd,
            "Please enter a valid salary greater than zero.",
            "Invalid Salary",
            MB_OK | MB_ICONWARNING);

        return;
    }

    employee.basicSalary = salary;

    /* Validate joining date */
    if (!IsValidDate(employee.joiningDate))
    {
        MessageBoxA(hwnd,
            "Enter a valid joining date in YYYY-MM-DD format.\n"
            "The date cannot be in the future.",
            "Invalid Joining Date",
            MB_OK | MB_ICONWARNING);

        return;
    }

    /* Check existing employees */
    int result = CheckExistingEmployees(&employee);

    if (result == 1)
    {
        MessageBoxA(hwnd,
            "This Employee ID already exists.",
            "Duplicate Employee",
            MB_OK | MB_ICONWARNING);

        return;
    }

    if (result == 2)
    {
        MessageBoxA(hwnd,
            "This National ID is already registered.",
            "Duplicate National ID",
            MB_OK | MB_ICONWARNING);

        return;
    }

    if (result == 3)
    {
        MessageBoxA(hwnd,
            "The system has reached its employee limit.",
            "Employee Limit",
            MB_OK | MB_ICONWARNING);

        return;
    }

    /* Set the employee's initial status */
    strcpy(employee.status, "Active");

    /* Ask for confirmation */
    int answer = MessageBoxA(hwnd,
        "Are you sure you want to register this employee?",
        "Confirm Registration",
        MB_YESNO | MB_ICONQUESTION);

    if (answer != IDYES)
        return;

    /* Save employee information */
    FILE *file = fopen("database/employees.dat", "ab");

    if (file == NULL)
    {
        MessageBoxA(hwnd,
            "Could not open the employee database.\n"
            "Please check that the database folder exists.",
            "File Error",
            MB_OK | MB_ICONERROR);

        return;
    }

    if (fwrite(&employee, sizeof(Employee), 1, file) != 1)
    {
        fclose(file);

        MessageBoxA(hwnd,
            "Employee information could not be saved.",
            "Save Error",
            MB_OK | MB_ICONERROR);

        return;
    }

    fclose(file);

/* Record employee registration in history */
AddHistoryRecord(
    "Employee Registered",
    employee.employeeID,
    "",
    "New employee registered successfully"
);

MessageBoxA(hwnd,
    "Employee registered successfully!",
    "Registration Successful",
    MB_OK | MB_ICONINFORMATION);

    /* Clear the form for the next employee */
    for (int i = 0; i < 9; i++)
    {
        if (i != 3 && i != 5)
            SetWindowTextA(fields[i], "");
    }

    SetFocus(fields[0]);
}

void TerminateSelectedEmployee(HWND hwnd);
void ShowUpdateEmployeeForm(HWND hwnd);
void SaveEmployeeUpdate(HWND hwnd);



/* Resize the employee table when its window changes size */
LRESULT CALLBACK EmployeeListProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_SIZE:
            if (employeeTable != NULL)
            {
                MoveWindow(employeeTable, 10, 55,
                    LOWORD(lParam) - 20,
                    HIWORD(lParam) - 65, TRUE);
            }
            return 0;

        case WM_COMMAND:

            if (LOWORD(wParam) == ID_UPDATE_EMPLOYEE)
{
            ShowUpdateEmployeeForm(hwnd);
              return 0;
}
            if (LOWORD(wParam) == ID_TERMINATE_EMPLOYEE)
{
                TerminateSelectedEmployee(hwnd);
              return 0;
}

            if (LOWORD(wParam) == ID_APPLY_MIN_WAGE)
            {
                char text[50];
                char *endPointer;

                GetWindowTextA(
                    minimumWageInput, text, sizeof(text));

                double amount = strtod(text, &endPointer);

                if (endPointer == text ||
                    *endPointer != '\0' ||
                    amount < 0)
                {
                    MessageBoxA(hwnd,
                        "Enter a valid minimum wage (zero or greater).",
                        "Invalid Amount",
                        MB_OK | MB_ICONWARNING);
                    return 0;
                }

                minimumWage = amount;

                InvalidateRect(employeeTable, NULL, TRUE);

                MessageBoxA(hwnd,
                    "Minimum wage updated successfully.",
                    "Success",
                    MB_OK | MB_ICONINFORMATION);

                return 0;
            }

            break;

        case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->hwndFrom == employeeTable &&
                header->code == NM_CUSTOMDRAW)
            {
                LPNMLVCUSTOMDRAW draw =
                    (LPNMLVCUSTOMDRAW)lParam;

                if (draw->nmcd.dwDrawStage == CDDS_PREPAINT)
                    return CDRF_NOTIFYITEMDRAW;

                if (draw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
                {
                    int row = (int)draw->nmcd.dwItemSpec;

                    char status[30] = "";
                    char type[30] = "";
                    char salaryText[40] = "";

                    ListView_GetItemText(
                        employeeTable, row, 9, status, sizeof(status));

                    ListView_GetItemText(
                        employeeTable, row, 5, type, sizeof(type));

                    ListView_GetItemText(
                        employeeTable, row, 8,
                        salaryText, sizeof(salaryText));

                    double salary = atof(salaryText);

                    if (strcmp(status, "Terminated") == 0)
                    {
                        draw->clrTextBk = RGB(210, 210, 210);
                        draw->clrText = RGB(100, 100, 100);
                    }
                    else if (minimumWage > 0 &&
                             salary < minimumWage)
                    {
                        draw->clrTextBk = RGB(255, 220, 220);
                        draw->clrText = RGB(180, 0, 0);
                    }
                    else if (strcmp(type, "Intern") == 0)
                    {
                        draw->clrTextBk = RGB(210, 230, 255);
                        draw->clrText = RGB(0, 50, 130);
                    }

                    return CDRF_DODEFAULT;
                }
            }

            break;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            listWindow = NULL;
            employeeTable = NULL;
            minimumWageInput = NULL;
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* Add one employee to the table */
void AddEmployeeRow(Employee employee)
{
    char salary[40];

    sprintf(salary, "%.2f", employee.basicSalary);

    LVITEMA item = {0};
    item.mask = LVIF_TEXT;
    item.iItem = ListView_GetItemCount(employeeTable);
    item.pszText = employee.employeeID;

    int row = ListView_InsertItem(employeeTable, &item);

    ListView_SetItemText(employeeTable, row, 1, employee.fullName);
    ListView_SetItemText(employeeTable, row, 2, employee.nationalID);
    ListView_SetItemText(employeeTable, row, 3, employee.department);
    ListView_SetItemText(employeeTable, row, 4, employee.jobTitle);
    ListView_SetItemText(employeeTable, row, 5, employee.employmentType);
    ListView_SetItemText(employeeTable, row, 6, employee.joiningDate);
    ListView_SetItemText(employeeTable, row, 7, employee.bankAccount);
    ListView_SetItemText(employeeTable, row, 8, salary);
    ListView_SetItemText(employeeTable, row, 9, employee.status);
}

/* Display all saved employees */
void ShowEmployeeList(HWND hwnd)
{
    if (listWindow != NULL && IsWindow(listWindow))
    {
        ShowWindow(listWindow, SW_SHOW);
        SetForegroundWindow(listWindow);
        return;
    }

    INITCOMMONCONTROLSEX controls = {0};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&controls);

    static int classRegistered = 0;

    if (!classRegistered)
    {
        WNDCLASSA wc = {0};

        wc.lpfnWndProc = EmployeeListProc;
        wc.hInstance = GetModuleHandleA(NULL);
        wc.lpszClassName = "EmployeeListWindow";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor = LoadCursorA(NULL, IDC_ARROW);

        if (!RegisterClassA(&wc))
        {
            MessageBoxA(hwnd, "Could not create employee list.",
                "Error", MB_OK | MB_ICONERROR);
            return;
        }

        classRegistered = 1;
    }

    listWindow = CreateWindowExA(
        0,
        "EmployeeListWindow",
        "Registered Employees",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1250, 550,
        hwnd, NULL,
        GetModuleHandleA(NULL), NULL
    );

    if (listWindow == NULL)
        return;

    CreateWindowA(
    "STATIC",
    "Minimum Wage:",
    WS_CHILD | WS_VISIBLE,
    10, 15, 110, 25,
    listWindow, NULL,
    GetModuleHandleA(NULL), NULL
);

minimumWageInput = CreateWindowExA(
    WS_EX_CLIENTEDGE,
    "EDIT",
    "0",
    WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
    120, 12, 150, 27,
    listWindow,
    (HMENU)(INT_PTR)ID_MIN_WAGE_INPUT,
    GetModuleHandleA(NULL), NULL
);

CreateWindowA(
    "BUTTON",
    "Apply",
    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
    285, 12, 100, 28,
    listWindow,
    (HMENU)(INT_PTR)ID_APPLY_MIN_WAGE,
    GetModuleHandleA(NULL), NULL
);

CreateWindowA(
    "BUTTON",
    "Terminate Selected",
    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
    400, 12, 150, 28,
    listWindow,
    (HMENU)(INT_PTR)ID_TERMINATE_EMPLOYEE,
    GetModuleHandleA(NULL), NULL
);

CreateWindowA(
    "BUTTON",
    "Update Selected",
    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
    560, 12, 140, 28,
    listWindow,
    (HMENU)(INT_PTR)ID_UPDATE_EMPLOYEE,
    GetModuleHandleA(NULL), NULL
);

    employeeTable = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        "SysListView32",
        "",
        WS_CHILD | WS_VISIBLE | LVS_REPORT |
        LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        10, 55, 1210, 440,
        listWindow, NULL,
        GetModuleHandleA(NULL), NULL
    );

    if (employeeTable == NULL)
    {
        DestroyWindow(listWindow);
        listWindow = NULL;
        return;
    }

    ListView_SetExtendedListViewStyle(
        employeeTable,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES
    );

    /* Create table columns */
    char *columnNames[] = {
        "Employee ID", "Full Name", "National ID",
        "Department", "Job Title", "Employment Type",
        "Joining Date", "Bank Account", "Basic Salary",
        "Status"
    };

    int columnWidths[] = {
        100, 150, 120, 100, 120,
        110, 100, 130, 100, 100
    };

    for (int i = 0; i < 10; i++)
    {
        LVCOLUMNA column = {0};
        column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        column.pszText = columnNames[i];
        column.cx = columnWidths[i];
        column.iSubItem = i;

        ListView_InsertColumn(employeeTable, i, &column);
    }

    /* Read employees from the database */
    FILE *file = fopen("database/employees.dat", "rb");

    if (file == NULL)
    {
        MessageBoxA(listWindow,
            "No employee database was found yet.",
            "Employee List",
            MB_OK | MB_ICONINFORMATION);
        return;
    }

    Employee employee;

    while (fread(&employee, sizeof(Employee), 1, file) == 1)
    {
        AddEmployeeRow(employee);
    }

    fclose(file);
}

void TerminateSelectedEmployee(HWND hwnd)
{
    int row = ListView_GetNextItem(
        employeeTable, -1, LVNI_SELECTED);

    if (row == -1)
    {
        MessageBoxA(hwnd,
            "Please select an employee first.",
            "No Employee Selected",
            MB_OK | MB_ICONWARNING);
        return;
    }

    char employeeID[30] = "";

    ListView_GetItemText(
        employeeTable, row, 0,
        employeeID, sizeof(employeeID));

    char message[150];

    sprintf(message,
        "Are you sure you want to terminate employee %s?",
        employeeID);

    if (MessageBoxA(hwnd, message,
        "Confirm Termination",
        MB_YESNO | MB_ICONQUESTION) != IDYES)
    {
        return;
    }

    FILE *file = fopen("database/employees.dat", "r+b");

    if (file == NULL)
    {
        MessageBoxA(hwnd,
            "Could not open the employee database.",
            "File Error",
            MB_OK | MB_ICONERROR);
        return;
    }

    Employee employee;
    int found = 0;

    while (fread(&employee, sizeof(Employee), 1, file) == 1)
    {
        if (strcmp(employee.employeeID, employeeID) == 0)
        {
            strcpy(employee.status, "Terminated");

            fseek(file, -(long)sizeof(Employee), SEEK_CUR);

            if (fwrite(&employee, sizeof(Employee), 1, file) == 1)
                found = 1;

            break;
        }
    }

    fclose(file);

    if (!found)
    {
        MessageBoxA(hwnd,
            "Employee could not be updated.",
            "Error",
            MB_OK | MB_ICONERROR);
        return;
    }

    /* Record employee termination in history */
    AddHistoryRecord(
    "Employee Terminated",
    employeeID,
    "",
    "Employee status changed to Terminated"
   );

    /* Refresh the table */
    ListView_DeleteAllItems(employeeTable);

    file = fopen("database/employees.dat", "rb");

    if (file != NULL)
    {
        while (fread(&employee, sizeof(Employee), 1, file) == 1)
        {
            AddEmployeeRow(employee);
        }

        fclose(file);
    }

    MessageBoxA(hwnd,
        "Employee has been terminated successfully.",
        "Success",
        MB_OK | MB_ICONINFORMATION);
}

/* Update form window procedure */
LRESULT CALLBACK UpdateEmployeeProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_SAVE_UPDATE)
            {
                SaveEmployeeUpdate(hwnd);
                return 0;
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            updateWindow = NULL;
            return 0;

        case WM_DESTROY:
            updateWindow = NULL;
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* Open the update form for the selected employee */
void ShowUpdateEmployeeForm(HWND hwnd)
{
    int row = ListView_GetNextItem(
        employeeTable, -1, LVNI_SELECTED);

    if (row == -1)
    {
        MessageBoxA(hwnd,
            "Please select an employee first.",
            "No Employee Selected",
            MB_OK | MB_ICONWARNING);
        return;
    }

    GetWindowTextA(employeeTable, updateEmployeeID,
                   sizeof(updateEmployeeID));

    ListView_GetItemText(employeeTable, row, 0,
                         updateEmployeeID,
                         sizeof(updateEmployeeID));

    Employee employee;
    int found = 0;

    FILE *file = fopen("database/employees.dat", "rb");

    if (file == NULL)
    {
        MessageBoxA(hwnd, "Could not open employee database.",
                    "File Error", MB_OK | MB_ICONERROR);
        return;
    }

    while (fread(&employee, sizeof(Employee), 1, file) == 1)
    {
        if (strcmp(employee.employeeID, updateEmployeeID) == 0)
        {
            found = 1;
            break;
        }
    }

    fclose(file);

    if (!found)
    {
        MessageBoxA(hwnd, "Employee record was not found.",
                    "Error", MB_OK | MB_ICONERROR);
        return;
    }

    if (updateWindow != NULL && IsWindow(updateWindow))
    {
        DestroyWindow(updateWindow);
    }

    static int classRegistered = 0;

    if (!classRegistered)
    {
        WNDCLASSA wc = {0};

        wc.lpfnWndProc = UpdateEmployeeProc;
        wc.hInstance = GetModuleHandleA(NULL);
        wc.lpszClassName = "UpdateEmployeeWindow";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor = LoadCursorA(NULL, IDC_ARROW);

        if (!RegisterClassA(&wc))
        {
            MessageBoxA(hwnd, "Could not create update form.",
                        "Error", MB_OK | MB_ICONERROR);
            return;
        }

        classRegistered = 1;
    }

    updateWindow = CreateWindowExA(
        0,
        "UpdateEmployeeWindow",
        "Update Employee",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        550, 450,
        hwnd, NULL,
        GetModuleHandleA(NULL), NULL
    );

    if (updateWindow == NULL)
        return;

    CreateLabel(updateWindow, "Department:", 30, 30, 140);

    const char *departments[] = {
        "Finance", "IT", "Sales", "Operations", "HR", NULL
    };

    updateFields[0] = CreateDropdown(
        updateWindow, ID_UPDATE_DEPARTMENT,
        180, 30, 280, departments);

    CreateLabel(updateWindow, "Job Title:", 30, 90, 140);

    updateFields[1] = CreateInput(
        updateWindow, ID_UPDATE_JOB, 180, 90, 280);

    CreateLabel(updateWindow, "Employment Type:", 30, 150, 140);

    const char *types[] = {
        "Permanent", "Contract", "Intern", NULL
    };

    updateFields[2] = CreateDropdown(
        updateWindow, ID_UPDATE_TYPE,
        180, 150, 280, types);

    CreateLabel(updateWindow, "Basic Salary:", 30, 210, 140);

    updateFields[3] = CreateInput(
        updateWindow, ID_UPDATE_SALARY, 180, 210, 280);

    CreateLabel(updateWindow, "Employee Status:", 30, 270, 140);

    const char *statuses[] = {
        "Active", "On Leave", "Terminated", NULL
    };

    updateFields[4] = CreateDropdown(
        updateWindow, ID_UPDATE_STATUS,
        180, 270, 280, statuses);

    /* Display the employee's current information */
    SendMessageA(updateFields[0], CB_SELECTSTRING,
                 (WPARAM)-1, (LPARAM)employee.department);

    SetWindowTextA(updateFields[1], employee.jobTitle);

    SendMessageA(updateFields[2], CB_SELECTSTRING,
                 (WPARAM)-1, (LPARAM)employee.employmentType);

    char salaryText[40];
    sprintf(salaryText, "%.2f", employee.basicSalary);

    SetWindowTextA(updateFields[3], salaryText);

    SendMessageA(updateFields[4], CB_SELECTSTRING,
                 (WPARAM)-1, (LPARAM)employee.status);

    CreateWindowA(
        "BUTTON", "Save Changes",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        180, 340, 150, 40,
        updateWindow,
        (HMENU)(INT_PTR)ID_SAVE_UPDATE,
        GetModuleHandleA(NULL), NULL
    );
}


/* Save the employee's updated information */
void SaveEmployeeUpdate(HWND hwnd)
{
    Employee employee;
    char salaryText[50];
    char *endPointer;
    double salary;

    char department[30];
    char jobTitle[50];
    char employmentType[20];
    char status[20];

    GetWindowTextA(updateFields[0], department, sizeof(department));
    GetWindowTextA(updateFields[1], jobTitle, sizeof(jobTitle));
    GetWindowTextA(updateFields[2], employmentType, sizeof(employmentType));
    GetWindowTextA(updateFields[3], salaryText, sizeof(salaryText));
    GetWindowTextA(updateFields[4], status, sizeof(status));

    if (IsBlank(department) ||
        IsBlank(jobTitle) ||
        IsBlank(employmentType) ||
        IsBlank(salaryText) ||
        IsBlank(status))
    {
        MessageBoxA(hwnd, "Please complete all fields.",
                    "Missing Information",
                    MB_OK | MB_ICONWARNING);
        return;
    }

    salary = strtod(salaryText, &endPointer);

    if (endPointer == salaryText ||
        *endPointer != '\0' || salary <= 0)
    {
        MessageBoxA(hwnd,
            "Please enter a valid salary greater than zero.",
            "Invalid Salary",
            MB_OK | MB_ICONWARNING);
        return;
    }

    FILE *file = fopen("database/employees.dat", "r+b");

    if (file == NULL)
    {
        MessageBoxA(hwnd, "Could not open employee database.",
                    "File Error", MB_OK | MB_ICONERROR);
        return;
    }

    int found = 0;
    double oldSalary = 0.0;

    while (fread(&employee, sizeof(Employee), 1, file) == 1)
    {
        if (strcmp(employee.employeeID, updateEmployeeID) == 0)
        {
            oldSalary = employee.basicSalary;
            strcpy(employee.department, department);
            strcpy(employee.jobTitle, jobTitle);
            strcpy(employee.employmentType, employmentType);
            strcpy(employee.status, status);
            employee.basicSalary = salary;

            fseek(file, -(long)sizeof(Employee), SEEK_CUR);

            if (fwrite(&employee, sizeof(Employee), 1, file) == 1)
                found = 1;

            break;
        }
    }

    fclose(file);

    if (!found)
    {
        MessageBoxA(hwnd, "Could not update employee record.",
                    "Update Error", MB_OK | MB_ICONERROR);
        return;
    }

    /* Record employee update in history */
char historyDetails[200];

if (oldSalary != salary)
{
    sprintf(
        historyDetails,
        "Employee details updated; salary changed from %.2f to %.2f",
        oldSalary,
        salary
    );
}
else
{
    strcpy(
        historyDetails,
        "Employee department, job title, employment type or status updated"
    );
}

AddHistoryRecord(
    "Employee Updated",
    updateEmployeeID,
    "",
    historyDetails
);

    /* Refresh the employee table */
    ListView_DeleteAllItems(employeeTable);

    file = fopen("database/employees.dat", "rb");

    if (file != NULL)
    {
        while (fread(&employee, sizeof(Employee), 1, file) == 1)
        {
            AddEmployeeRow(employee);
        }

        fclose(file);
    }

    MessageBoxA(hwnd,
        "Employee information updated successfully!",
        "Update Successful",
        MB_OK | MB_ICONINFORMATION);

    DestroyWindow(hwnd);
}
