
#ifndef PAYROLL_H
#define PAYROLL_H

#include <windows.h>

typedef struct
{
    char employeeID[20];
    char payMonth[10];

    double basicSalary;
    double housingAllowance;
    double transportAllowance;
    double medicalAllowance;
    double performanceBonus;
    double overtimePay;

    double grossSalary;

    double incomeTax;
    double socialSecurity;
    double absenceDeduction;
    double loanRepayment;

    double totalDeductions;
    double netSalary;

} Payroll;

void ShowPayrollForm(HWND hwnd);

#endif