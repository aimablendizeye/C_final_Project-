

#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "payslip.h"
#include "payroll.h"
#include "employee.h"
#include "history.h"

#define ID_SLIP_EMPLOYEE 501
#define ID_SLIP_MONTH    502
#define ID_SLIP_VIEW     503
#define ID_SLIP_EXPORT   504

HWND payslipWindow = NULL;
HWND slipEmployeeField;
HWND slipMonthField;

Payroll currentPayroll;
Employee currentEmployee;
int payslipFound = 0;

/* Find saved payroll */
int FindSavedPayroll(const char *id, const char *month)
{
    FILE *file = fopen("database/payroll.dat", "rb");

    if (file == NULL)
        return 0;

    Payroll p;

    while (fread(&p, sizeof(Payroll), 1, file) == 1)
    {
        if (strcmp(p.employeeID, id) == 0 &&
            strcmp(p.payMonth, month) == 0)
        {
            currentPayroll = p;
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}

/* Find employee details */
int FindSlipEmployee(const char *id)
{
    FILE *file = fopen("database/employees.dat", "rb");

    if (file == NULL)
        return 0;

    Employee e;

    while (fread(&e, sizeof(Employee), 1, file) == 1)
    {
        if (strcmp(e.employeeID, id) == 0)
        {
            currentEmployee = e;
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}

/* Create payslip text */
void CreatePayslipText(char *message, int size)
{
    Payroll p = currentPayroll;
    Employee e = currentEmployee;

    snprintf(message, size,
        "          EMPLOYEE PAYSLIP\n"
        "====================================\n"
        "Employee ID: %s\n"
        "Employee Name: %s\n"
        "Department: %s\n"
        "Job Title: %s\n"
        "Pay Month: %s\n"
        "====================================\n"
        "EARNINGS\n"
        "Basic Salary:          %.2f\n"
        "Housing Allowance:     %.2f\n"
        "Transport Allowance:   %.2f\n"
        "Medical Allowance:     %.2f\n"
        "Performance Bonus:     %.2f\n"
        "Overtime Pay:          %.2f\n"
        "------------------------------------\n"
        "Gross Salary:          %.2f\n"
        "====================================\n"
        "DEDUCTIONS\n"
        "Income Tax:            %.2f\n"
        "Social Security:       %.2f\n"
        "Absence Deduction:     %.2f\n"
        "Loan Repayment:        %.2f\n"
        "------------------------------------\n"
        "Total Deductions:      %.2f\n"
        "====================================\n"
        "NET SALARY:            %.2f\n"
        "====================================\n"
        "Thank you.\n",
        e.employeeID, e.fullName, e.department,
        e.jobTitle, p.payMonth,
        p.basicSalary, p.housingAllowance,
        p.transportAllowance, p.medicalAllowance,
        p.performanceBonus, p.overtimePay,
        p.grossSalary, p.incomeTax,
        p.socialSecurity, p.absenceDeduction,
        p.loanRepayment, p.totalDeductions,
        p.netSalary);
}

/* Display payslip */
void ViewPayslip(HWND hwnd)
{
    char id[20], month[10];

    GetWindowTextA(slipEmployeeField, id, sizeof(id));
    GetWindowTextA(slipMonthField, month, sizeof(month));

    if (strlen(id) == 0 || strlen(month) == 0)
    {
        MessageBoxA(hwnd, "Enter employee ID and pay month.",
                    "Missing Information", MB_OK | MB_ICONWARNING);
        return;
    }

    if (!FindSavedPayroll(id, month))
    {
        payslipFound = 0;

        MessageBoxA(hwnd, "No payroll record found for this employee and month.",
                    "Payslip Not Found", MB_OK | MB_ICONWARNING);
        return;
    }

    if (!FindSlipEmployee(id))
    {
        payslipFound = 0;

        MessageBoxA(hwnd, "Employee details could not be found.",
                    "Error", MB_OK | MB_ICONERROR);
        return;
    }

    payslipFound = 1;

    char message[3000];

    CreatePayslipText(message, sizeof(message));

    MessageBoxA(hwnd, message, "Employee Payslip",
                MB_OK | MB_ICONINFORMATION);
}

/* Export payslip to text file */
void ExportPayslip(HWND hwnd)
{
    if (!payslipFound)
    {
        MessageBoxA(hwnd, "Please view a payslip before exporting.",
                    "No Payslip", MB_OK | MB_ICONWARNING);
        return;
    }

    char filename[100];

    snprintf(filename, sizeof(filename),
        "payslips/%s_%s.txt",
        currentPayroll.employeeID,
        currentPayroll.payMonth);

    /* Replace month hyphen in filename */
    for (int i = 0; filename[i] != '\0'; i++)
    {
        if (filename[i] == '-')
            filename[i] = '_';
    }

    FILE *file = fopen(filename, "w");

    if (file == NULL)
    {
        MessageBoxA(hwnd,
            "Could not create payslip file. Check the payslips folder.",
            "File Error", MB_OK | MB_ICONERROR);
        return;
    }

    char message[3000];

    CreatePayslipText(message, sizeof(message));

  fprintf(file, "%s", message);

fclose(file);

/* Record this activity in history */
AddHistoryRecord(
    "Payslip Exported",
    currentPayroll.employeeID,
    currentPayroll.payMonth,
    "Payslip exported to text file"
);

MessageBoxA(hwnd,
    "Payslip exported successfully!",
        "Export Complete", MB_OK | MB_ICONINFORMATION);
}

/* Payslip window procedure */
LRESULT CALLBACK PayslipWindowProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_COMMAND:

            if (LOWORD(wParam) == ID_SLIP_VIEW)
            {
                ViewPayslip(hwnd);
                return 0;
            }

            if (LOWORD(wParam) == ID_SLIP_EXPORT)
            {
                ExportPayslip(hwnd);
                return 0;
            }

            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            payslipWindow = NULL;
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* Open payslip form */
void ShowPayslipForm(HWND hwnd)
{
    if (payslipWindow != NULL && IsWindow(payslipWindow))
    {
        ShowWindow(payslipWindow, SW_SHOW);
        SetForegroundWindow(payslipWindow);
        return;
    }

    static int registered = 0;

    if (!registered)
    {
        WNDCLASSA wc = {0};

        wc.lpfnWndProc = PayslipWindowProc;
        wc.hInstance = GetModuleHandleA(NULL);
        wc.lpszClassName = "PayslipWindow";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor = LoadCursorA(NULL, IDC_ARROW);

        if (!RegisterClassA(&wc))
        {
            MessageBoxA(hwnd, "Could not create payslip window.",
                        "Error", MB_OK | MB_ICONERROR);
            return;
        }

        registered = 1;
    }

    payslipWindow = CreateWindowExA(
        0, "PayslipWindow", "Employee Payslip",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        500, 330,
        hwnd, NULL, GetModuleHandleA(NULL), NULL
    );

    if (payslipWindow == NULL)
        return;

    CreateWindowA(
        "STATIC", "Employee ID:",
        WS_CHILD | WS_VISIBLE,
        30, 40, 140, 25,
        payslipWindow, NULL, NULL, NULL
    );

    slipEmployeeField = CreateWindowExA(
        WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        180, 40, 220, 27,
        payslipWindow, (HMENU)(INT_PTR)ID_SLIP_EMPLOYEE,
        GetModuleHandleA(NULL), NULL
    );

    CreateWindowA(
        "STATIC", "Pay Month (MM-YYYY):",
        WS_CHILD | WS_VISIBLE,
        30, 95, 140, 25,
        payslipWindow, NULL, NULL, NULL
    );

    slipMonthField = CreateWindowExA(
        WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        180, 95, 220, 27,
        payslipWindow, (HMENU)(INT_PTR)ID_SLIP_MONTH,
        GetModuleHandleA(NULL), NULL
    );

    CreateWindowA(
        "BUTTON", "View Payslip",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        70, 170, 150, 40,
        payslipWindow, (HMENU)(INT_PTR)ID_SLIP_VIEW,
        GetModuleHandleA(NULL), NULL
    );

    CreateWindowA(
        "BUTTON", "Export Payslip",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        250, 170, 150, 40,
        payslipWindow, (HMENU)(INT_PTR)ID_SLIP_EXPORT,
        GetModuleHandleA(NULL), NULL
    );
}