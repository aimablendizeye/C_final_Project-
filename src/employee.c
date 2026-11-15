

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

/* IDs for form fields */
#define ID_VIEW_EMPLOYEES 212

HWND listWindow = NULL;
HWND employeeTable = NULL;

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



/* Resize the employee table when its window changes size */
LRESULT CALLBACK EmployeeListProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_SIZE:
            if (employeeTable != NULL)
            {
                MoveWindow(employeeTable, 10, 10,
                    LOWORD(lParam) - 20,
                    HIWORD(lParam) - 20, TRUE);
            }
            return 0;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            listWindow = NULL;
            employeeTable = NULL;
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

    employeeTable = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        "SysListView32",
        "",
        WS_CHILD | WS_VISIBLE | LVS_REPORT |
        LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        10, 10, 1210, 500,
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
