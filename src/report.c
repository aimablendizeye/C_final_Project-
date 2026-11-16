

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "report.h"
#include "employee.h"
#include "attendance.h"
#include "payroll.h"

#define ID_MONTH_INPUT 301
#define ID_GENERATE_REPORT 302
#define ID_REPORT_LIST 303
#define ID_EXPORT_REPORT 304

#define MAX_REPORT_EMPLOYEES 500

static HWND hMonthInput;
static HWND hReportList;
static char currentReportMonth[10] = "";

/* Department names */
static const char *departments[] = {
    "Finance",
    "IT",
    "Sales",
    "Operations",
    "HR"
};

#define DEPARTMENT_COUNT 5

/* Find an employee using their ID */
static int FindEmployee(
    Employee employees[],
    int count,
    const char *employeeID)
{
    for (int i = 0; i < count; i++)
    {
        if (strcmp(employees[i].employeeID, employeeID) == 0)
            return i;
    }

    return -1;
}

/* Validate MM-YYYY */
static int IsValidMonth(const char *month)
{
    if (strlen(month) != 7 || month[2] != '-')
        return 0;

    if (month[0] < '0' || month[0] > '9' ||
        month[1] < '0' || month[1] > '9')
        return 0;

    for (int i = 3; i < 7; i++)
    {
        if (month[i] < '0' || month[i] > '9')
            return 0;
    }

    int mm = (month[0] - '0') * 10 + (month[1] - '0');
    int yyyy = atoi(month + 3);

    return mm >= 1 && mm <= 12 && yyyy >= 1900;
}

/* Add a line to the report list */
static void AddReportLine(const char *text)
{
    SendMessageA(
        hReportList,
        LB_ADDSTRING,
        0,
        (LPARAM)text
    );
}

/* Generate the monthly payroll report */

static void GenerateMonthlyReport(HWND hwnd)
{
    char month[10] = {0};

    GetWindowTextA(hMonthInput, month, sizeof(month));

    if (!IsValidMonth(month))
    {
        MessageBoxA(
            hwnd,
            "Enter the month in MM-YYYY format.",
            "Invalid Month",
            MB_OK | MB_ICONWARNING
        );
        return;
    }

    strcpy(currentReportMonth, month);

    SendMessageA(hReportList, LB_RESETCONTENT, 0, 0);

    Employee employees[MAX_REPORT_EMPLOYEES];
    int employeeCount = 0;

    /* Load employees */
    FILE *employeeFile = fopen("database/employees.dat", "rb");

    if (employeeFile != NULL)
    {
        while (employeeCount < MAX_REPORT_EMPLOYEES &&
               fread(&employees[employeeCount],
                     sizeof(Employee), 1, employeeFile) == 1)
        {
            employeeCount++;
        }

        fclose(employeeFile);
    }

    /* Count active employees by department */
    int activeCount = 0;
    int departmentCounts[DEPARTMENT_COUNT] = {0};

    for (int i = 0; i < employeeCount; i++)
    {
        if (strcmp(employees[i].status, "Active") == 0)
        {
            activeCount++;

            for (int d = 0; d < DEPARTMENT_COUNT; d++)
            {
                if (strcmp(employees[i].department, departments[d]) == 0)
                    departmentCounts[d]++;
            }
        }
    }

    /* Payroll totals */
    double totalGross = 0.0;
    double totalDeductions = 0.0;
    double totalNet = 0.0;
    double totalTax = 0.0;
    double totalSocialSecurity = 0.0;

    double departmentCosts[DEPARTMENT_COUNT] = {0};

    double highestNet = -1.0;
    char highestEmployeeID[20] = "None";
    char highestEmployeeName[100] = "None";

    int payrollCount = 0;

    FILE *payrollFile = fopen("database/payroll.dat", "rb");

    if (payrollFile != NULL)
    {
        Payroll p;

        while (fread(&p, sizeof(Payroll), 1, payrollFile) == 1)
        {
            if (strcmp(p.payMonth, month) != 0)
                continue;

            payrollCount++;

            totalGross += p.grossSalary;
            totalDeductions += p.totalDeductions;
            totalNet += p.netSalary;
            totalTax += p.incomeTax;
            totalSocialSecurity += p.socialSecurity;

            int index = FindEmployee(
                employees,
                employeeCount,
                p.employeeID
            );

            if (index >= 0)
            {
                for (int d = 0; d < DEPARTMENT_COUNT; d++)
                {
                    if (strcmp(employees[index].department,
                               departments[d]) == 0)
                    {
                        departmentCosts[d] += p.grossSalary;
                    }
                }

                if (p.netSalary > highestNet)
                {
                    highestNet = p.netSalary;

                    strcpy(highestEmployeeID, p.employeeID);
                    strcpy(highestEmployeeName,
                           employees[index].fullName);
                }
            }
        }

        fclose(payrollFile);
    }

    /* Find department with highest payroll cost */
    int highestDepartment = -1;
    double highestDepartmentCost = 0.0;

    for (int d = 0; d < DEPARTMENT_COUNT; d++)
    {
        if (departmentCosts[d] > highestDepartmentCost)
        {
            highestDepartmentCost = departmentCosts[d];
            highestDepartment = d;
        }
    }

    /* Display report */
    char line[250];

    AddReportLine("========== MONTHLY PAYROLL REPORT ==========");

    sprintf(line, "Pay Month: %s", month);
    AddReportLine(line);

    AddReportLine("");
    AddReportLine("EMPLOYEE SUMMARY");
    AddReportLine("--------------------------------------------");

    sprintf(line, "Total Active Employees: %d", activeCount);
    AddReportLine(line);

    for (int d = 0; d < DEPARTMENT_COUNT; d++)
    {
        sprintf(line, "%s Department: %d employees",
                departments[d], departmentCounts[d]);
        AddReportLine(line);
    }

    AddReportLine("");
    AddReportLine("PAYROLL SUMMARY");
    AddReportLine("--------------------------------------------");

    sprintf(line, "Payroll Records: %d", payrollCount);
    AddReportLine(line);

    sprintf(line, "Total Gross Salary: %.2f", totalGross);
    AddReportLine(line);

    sprintf(line, "Total Deductions: %.2f", totalDeductions);
    AddReportLine(line);

    sprintf(line, "Total Net Salary Paid: %.2f", totalNet);
    AddReportLine(line);

    sprintf(line, "Income Tax Remittance: %.2f", totalTax);
    AddReportLine(line);

    sprintf(line, "Social Security Remittance: %.2f",
            totalSocialSecurity);
    AddReportLine(line);

    AddReportLine("");
    AddReportLine("PAYROLL HIGHLIGHTS");
    AddReportLine("--------------------------------------------");

    if (highestNet >= 0)
    {
        sprintf(line, "Highest Paid Employee: %s (%s)",
                highestEmployeeName, highestEmployeeID);
        AddReportLine(line);

        sprintf(line, "Highest Net Salary: %.2f", highestNet);
        AddReportLine(line);
    }
    else
    {
        AddReportLine("Highest Paid Employee: No payroll records");
    }

    if (highestDepartment >= 0)
    {
        sprintf(line, "Highest Payroll Cost Department: %s",
                departments[highestDepartment]);
        AddReportLine(line);

        sprintf(line, "Department Gross Payroll Cost: %.2f",
                highestDepartmentCost);
        AddReportLine(line);
    }
    else
    {
        AddReportLine("Department Payroll Cost: No records");
    }

    AddReportLine("");
    AddReportLine("EMPLOYEES WITH MORE THAN 3 ABSENT DAYS");
    AddReportLine("--------------------------------------------");

    int absenceFound = 0;

    FILE *attendanceFile = fopen("database/attendance.dat", "rb");

    if (attendanceFile != NULL)
    {
        Attendance a;

        while (fread(&a, sizeof(Attendance), 1, attendanceFile) == 1)
        {
            if (strcmp(a.payMonth, month) == 0 &&
                a.daysAbsent > 3)
            {
                int index = FindEmployee(
                    employees,
                    employeeCount,
                    a.employeeID
                );

                if (index >= 0)
                {
                    sprintf(line, "%s (%s): %d absent days",
                            employees[index].fullName,
                            a.employeeID,
                            a.daysAbsent);
                }
                else
                {
                    sprintf(line, "Employee %s: %d absent days",
                            a.employeeID,
                            a.daysAbsent);
                }

                AddReportLine(line);
                absenceFound = 1;
            }
        }

        fclose(attendanceFile);
    }

    if (!absenceFound)
        AddReportLine("No employees with more than 3 absent days.");

    AddReportLine("");
    AddReportLine("========== END OF REPORT ==========");

    if (payrollCount == 0)
    {
        MessageBoxA(
            hwnd,
            "No payroll records were found for this month.",
            "Report Generated",
            MB_OK | MB_ICONINFORMATION
        );
    }
}

/* Export the displayed report to a text file */
static void ExportMonthlyReport(HWND hwnd)
{
    if (currentReportMonth[0] == '\0')
    {
        MessageBoxA(
            hwnd,
            "Please generate a report first.",
            "No Report",
            MB_OK | MB_ICONWARNING
        );
        return;
    }

    CreateDirectoryA("reports", NULL);

    char filename[100];

    sprintf(
        filename,
        "reports/payroll_report_%s.txt",
        currentReportMonth
    );

    FILE *file = fopen(filename, "w");

    if (file == NULL)
    {
        MessageBoxA(
            hwnd,
            "Could not create the report file.",
            "Export Error",
            MB_OK | MB_ICONERROR
        );
        return;
    }

    int count = (int)SendMessageA(
        hReportList,
        LB_GETCOUNT,
        0,
        0
    );

    char line[500];

    for (int i = 0; i < count; i++)
    {
        SendMessageA(
            hReportList,
            LB_GETTEXT,
            (WPARAM)i,
            (LPARAM)line
        );

        fprintf(file, "%s\n", line);
    }

    fclose(file);

    MessageBoxA(
        hwnd,
        "Monthly report exported successfully!",
        "Export Successful",
        MB_OK | MB_ICONINFORMATION
    );
}

/* Report window procedure */
static LRESULT CALLBACK ReportWindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
        {
            CreateWindowExA(
                0, "STATIC", "Enter Pay Month (MM-YYYY):",
                WS_CHILD | WS_VISIBLE,
                20, 20, 220, 25,
                hwnd, NULL, NULL, NULL
            );

            hMonthInput = CreateWindowExA(
                WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                245, 18, 130, 28,
                hwnd, (HMENU)(INT_PTR)ID_MONTH_INPUT,
                NULL, NULL
            );

            CreateWindowExA(
                0, "BUTTON", "Generate Report",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                390, 17, 150, 30,
                hwnd, (HMENU)(INT_PTR)ID_GENERATE_REPORT,
                NULL, NULL
            );

            CreateWindowExA(
              0, "BUTTON", "Export Report",
              WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
              555, 17, 150, 30,
              hwnd, (HMENU)(INT_PTR)ID_EXPORT_REPORT,
              NULL, NULL
              );

            hReportList = CreateWindowExA(
                WS_EX_CLIENTEDGE,
                "LISTBOX",
                "",
                WS_CHILD | WS_VISIBLE |
                WS_VSCROLL | WS_HSCROLL |
                LBS_NOINTEGRALHEIGHT,
                20, 65, 740, 480,
                hwnd, (HMENU)(INT_PTR)ID_REPORT_LIST,
                NULL, NULL
            );

            return 0;
        }

      case WM_COMMAND:
           if (LOWORD(wParam) == ID_GENERATE_REPORT)
             {
           GenerateMonthlyReport(hwnd);
           return 0;
          }

           if (LOWORD(wParam) == ID_EXPORT_REPORT)
          {
            ExportMonthlyReport(hwnd);
          return 0;
                }
             break;

        case WM_SIZE:
            if (hReportList != NULL)
            {
                int width = LOWORD(lParam);
                int height = HIWORD(lParam);

                MoveWindow(
                    hReportList,
                    20, 65,
                    width - 40,
                    height - 90,
                    TRUE
                );
            }
            return 0;

        case WM_DESTROY:
            hMonthInput = NULL;
            hReportList = NULL;
            return 0;
    }

    return DefWindowProcA(hwnd, message, wParam, lParam);
}

/* Open the report window */
void ShowReportForm(HWND hwnd)
{
    const char CLASS_NAME[] = "PayrollReportWindow";

    WNDCLASSA wc = {0};

    wc.lpfnWndProc = ReportWindowProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!GetClassInfoA(wc.hInstance, CLASS_NAME, &wc))
    {
        RegisterClassA(&wc);
    }

    HWND reportWindow = CreateWindowExA(
        0,
        CLASS_NAME,
        "Monthly Payroll Reports",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        820, 650,
        hwnd,
        NULL,
        GetModuleHandleA(NULL),
        NULL
    );

    if (reportWindow != NULL)
    {
        ShowWindow(reportWindow, SW_SHOW);
        UpdateWindow(reportWindow);
    }
}
