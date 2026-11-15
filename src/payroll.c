
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "payroll.h"
#include "employee.h"
#include "attendance.h"
#include "history.h"

#define ID_PAY_EMPLOYEE 401
#define ID_PAY_MONTH    402
#define ID_PAY_TRANSPORT 403
#define ID_PAY_MEDICAL  404
#define ID_PAY_BONUS    405
#define ID_PAY_LOAN     406
#define ID_PAY_CALCULATE 407

HWND payrollWindow = NULL;
HWND payrollFields[6];

/* Create a label */
void PayrollLabel(HWND hwnd, const char *text, int x, int y)
{
    CreateWindowA(
        "STATIC", text,
        WS_CHILD | WS_VISIBLE,
        x, y, 170, 25,
        hwnd, NULL, NULL, NULL
    );
}

/* Create an input */
HWND PayrollInput(HWND hwnd, int id, int x, int y)
{
    return CreateWindowExA(
        WS_EX_CLIENTEDGE,
        "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        x, y, 220, 27,
        hwnd,
        (HMENU)(INT_PTR)id,
        GetModuleHandleA(NULL), NULL
    );
}

/* Check month format MM-YYYY */
int PayrollValidMonth(const char *month)
{
    if (strlen(month) != 7 || month[2] != '-')
        return 0;

    for (int i = 0; i < 7; i++)
    {
        if (i != 2 && !isdigit((unsigned char)month[i]))
            return 0;
    }

    int m = (month[0] - '0') * 10 + month[1] - '0';
    int y = atoi(month + 3);

    return m >= 1 && m <= 12 && y >= 1;
}

/* Read a non-negative number */
int ReadMoney(HWND hwnd, HWND field, double *value)
{
    char text[50];
    char *end;

    GetWindowTextA(field, text, sizeof(text));

    if (text[0] == '\0')
        return 0;

    *value = strtod(text, &end);

    if (end == text || *end != '\0' || *value < 0)
        return 0;

    return 1;
}

/* Find an employee using their ID */
int FindPayrollEmployee(const char *id, Employee *employee)
{
    FILE *file = fopen("database/employees.dat", "rb");

    if (file == NULL)
        return 0;

    while (fread(employee, sizeof(Employee), 1, file) == 1)
    {
        if (strcmp(employee->employeeID, id) == 0)
        {
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}

/* Find attendance for the employee and month */
int FindPayrollAttendance(
    const char *id,
    const char *month,
    Attendance *attendance)
{
    FILE *file = fopen("database/attendance.dat", "rb");

    if (file == NULL)
        return 0;

    while (fread(attendance, sizeof(Attendance), 1, file) == 1)
    {
        if (strcmp(attendance->employeeID, id) == 0 &&
            strcmp(attendance->payMonth, month) == 0)
        {
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}

/* Prevent duplicate payroll */
int PayrollAlreadyExists(const char *id, const char *month)
{
    FILE *file = fopen("database/payroll.dat", "rb");

    if (file == NULL)
        return 0;

    Payroll record;

    while (fread(&record, sizeof(Payroll), 1, file) == 1)
    {
        if (strcmp(record.employeeID, id) == 0 &&
            strcmp(record.payMonth, month) == 0)
        {
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}

/* Calculate progressive income tax */
double CalculateIncomeTax(double gross)
{
    double tax = 0;

    if (gross > 100000)
    {
        tax += (gross - 100000) * 0.30;
        tax += 40000 * 0.20;
    }
    else if (gross > 60000)
    {
        tax += (gross - 60000) * 0.20;
    }

    return tax;
}

/* Calculate and save payroll */
void CalculateAndSavePayroll(HWND hwnd)
{
    char employeeID[20];
    char month[10];

    double transport, medical, bonusPercent, loan;

    GetWindowTextA(payrollFields[0], employeeID, sizeof(employeeID));
    GetWindowTextA(payrollFields[1], month, sizeof(month));

    if (employeeID[0] == '\0' || month[0] == '\0')
    {
        MessageBoxA(hwnd, "Enter employee ID and pay month.",
                    "Missing Information", MB_OK | MB_ICONWARNING);
        return;
    }

    if (!PayrollValidMonth(month))
    {
        MessageBoxA(hwnd, "Use the month format MM-YYYY.",
                    "Invalid Month", MB_OK | MB_ICONWARNING);
        return;
    }

    if (!ReadMoney(hwnd, payrollFields[2], &transport) ||
        !ReadMoney(hwnd, payrollFields[3], &medical) ||
        !ReadMoney(hwnd, payrollFields[4], &bonusPercent) ||
        !ReadMoney(hwnd, payrollFields[5], &loan))
    {
        MessageBoxA(hwnd, "Enter valid non-negative amounts in all allowance and loan fields.",
                    "Invalid Amount", MB_OK | MB_ICONWARNING);
        return;
    }

    if (bonusPercent > 20)
    {
        MessageBoxA(hwnd, "Performance bonus must be between 0 and 20 percent.",
                    "Invalid Bonus", MB_OK | MB_ICONWARNING);
        return;
    }

    if (PayrollAlreadyExists(employeeID, month))
    {
        MessageBoxA(hwnd, "Payroll already exists for this employee and month.",
                    "Duplicate Payroll", MB_OK | MB_ICONWARNING);
        return;
    }

    Employee employee = {0};

    if (!FindPayrollEmployee(employeeID, &employee))
    {
        MessageBoxA(hwnd, "Employee ID was not found.",
                    "Employee Not Found", MB_OK | MB_ICONWARNING);
        return;
    }

    if (strcmp(employee.status, "Terminated") == 0)
    {
        MessageBoxA(hwnd, "A terminated employee cannot receive payroll.",
                    "Terminated Employee", MB_OK | MB_ICONWARNING);
        return;
    }

    Attendance attendance = {0};

    if (!FindPayrollAttendance(employeeID, month, &attendance))
    {
        MessageBoxA(hwnd, "No attendance record was found for this employee and month.",
                    "Attendance Missing", MB_OK | MB_ICONWARNING);
        return;
    }

    Payroll p = {0};

    strcpy(p.employeeID, employeeID);
    strcpy(p.payMonth, month);

    p.basicSalary = employee.basicSalary;

    /* Allowances */
    p.housingAllowance = p.basicSalary * 0.15;
    p.transportAllowance = transport;
    p.medicalAllowance = medical;
    p.performanceBonus = p.basicSalary * bonusPercent / 100.0;

    /* Overtime */
    p.overtimePay =
        attendance.overtimeHours * (p.basicSalary / 176.0) * 1.5;

    /* Gross salary */
    p.grossSalary =
        p.basicSalary +
        p.housingAllowance +
        p.transportAllowance +
        p.medicalAllowance +
        p.performanceBonus +
        p.overtimePay;

    /* Deductions */
    p.incomeTax = CalculateIncomeTax(p.grossSalary);
    p.socialSecurity = p.basicSalary * 0.03;

    p.absenceDeduction =
        attendance.daysAbsent * (p.basicSalary / 22.0);

    p.loanRepayment = loan;

    p.totalDeductions =
        p.incomeTax +
        p.socialSecurity +
        p.absenceDeduction +
        p.loanRepayment;

    if (p.totalDeductions > p.grossSalary)
    {
        MessageBoxA(hwnd,
            "Total deductions are greater than gross salary. Payroll was not saved.",
            "Invalid Payroll", MB_OK | MB_ICONWARNING);
        return;
    }

    p.netSalary = p.grossSalary - p.totalDeductions;

    /* Save payroll */
    FILE *file = fopen("database/payroll.dat", "ab");

    if (file == NULL)
    {
        MessageBoxA(hwnd, "Could not open payroll database.",
                    "File Error", MB_OK | MB_ICONERROR);
        return;
    }

    if (fwrite(&p, sizeof(Payroll), 1, file) != 1)
    {
        fclose(file);

        MessageBoxA(hwnd, "Payroll could not be saved.",
                    "Save Error", MB_OK | MB_ICONERROR);
        return;
    }

  fclose(file);

/* Record payroll activity in history */
AddHistoryRecord(
    "Payroll Calculated",
    p.employeeID,
    p.payMonth,
    "Monthly payroll saved"
);

/* Display salary breakdown */
char result[1200];

    snprintf(result, sizeof(result),
        "PAYROLL CALCULATED SUCCESSFULLY\n\n"
        "Employee: %s\n"
        "Pay Month: %s\n\n"
        "BASIC SALARY: %.2f\n"
        "Housing Allowance: %.2f\n"
        "Transport Allowance: %.2f\n"
        "Medical Allowance: %.2f\n"
        "Performance Bonus: %.2f\n"
        "Overtime Pay: %.2f\n\n"
        "GROSS SALARY: %.2f\n\n"
        "Income Tax: %.2f\n"
        "Social Security: %.2f\n"
        "Absence Deduction: %.2f\n"
        "Loan Repayment: %.2f\n"
        "Total Deductions: %.2f\n\n"
        "NET SALARY: %.2f",
        employee.fullName,
        month,
        p.basicSalary,
        p.housingAllowance,
        p.transportAllowance,
        p.medicalAllowance,
        p.performanceBonus,
        p.overtimePay,
        p.grossSalary,
        p.incomeTax,
        p.socialSecurity,
        p.absenceDeduction,
        p.loanRepayment,
        p.totalDeductions,
        p.netSalary
    );

    MessageBoxA(hwnd, result, "Payroll Breakdown",
                MB_OK | MB_ICONINFORMATION);

    for (int i = 0; i < 6; i++)
        SetWindowTextA(payrollFields[i], "");
}

/* Payroll window procedure */
LRESULT CALLBACK PayrollWindowProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_PAY_CALCULATE)
            {
                CalculateAndSavePayroll(hwnd);
                return 0;
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            payrollWindow = NULL;
            return 0;

        case WM_DESTROY:
            payrollWindow = NULL;
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* Open payroll form */
void ShowPayrollForm(HWND hwnd)
{
    if (payrollWindow != NULL && IsWindow(payrollWindow))
    {
        ShowWindow(payrollWindow, SW_SHOW);
        SetForegroundWindow(payrollWindow);
        return;
    }

    static int classRegistered = 0;

    if (!classRegistered)
    {
        WNDCLASSA wc = {0};

        wc.lpfnWndProc = PayrollWindowProc;
        wc.hInstance = GetModuleHandleA(NULL);
        wc.lpszClassName = "PayrollCalculationWindow";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor = LoadCursorA(NULL, IDC_ARROW);

        if (!RegisterClassA(&wc))
        {
            MessageBoxA(hwnd, "Could not create payroll window.",
                        "Error", MB_OK | MB_ICONERROR);
            return;
        }

        classRegistered = 1;
    }

    payrollWindow = CreateWindowExA(
        0,
        "PayrollCalculationWindow",
        "Monthly Payroll Calculation",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        520, 570,
        hwnd, NULL,
        GetModuleHandleA(NULL), NULL
    );

    if (payrollWindow == NULL)
        return;

    const char *labels[] = {
        "Employee ID:",
        "Pay Month (MM-YYYY):",
        "Transport Allowance:",
        "Medical Allowance:",
        "Performance Bonus (%):",
        "Loan Repayment:"
    };

    for (int i = 0; i < 6; i++)
    {
        int y = 30 + i * 60;

        PayrollLabel(payrollWindow, labels[i], 30, y);

        payrollFields[i] =
            PayrollInput(payrollWindow, ID_PAY_EMPLOYEE + i, 210, y);
    }

    CreateWindowA(
        "BUTTON", "Calculate and Save Payroll",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        150, 420, 220, 40,
        payrollWindow,
        (HMENU)(INT_PTR)ID_PAY_CALCULATE,
        GetModuleHandleA(NULL), NULL
    );
}