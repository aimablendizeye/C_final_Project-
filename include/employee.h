

#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include <windows.h>

#define MAX_EMPLOYEES 500

typedef struct
{
    char employeeID[20];
    char fullName[100];
    char nationalID[30];
    char department[30];
    char jobTitle[50];
    char employmentType[20];
    char joiningDate[15];
    char bankAccount[30];

    double basicSalary;

    char status[20];

} Employee;

/* Employee management functions */
void ShowEmployeeForm(HWND hwnd);
void RegisterEmployee(HWND hwnd);

void ShowEmployeeList(HWND hwnd);

#endif
