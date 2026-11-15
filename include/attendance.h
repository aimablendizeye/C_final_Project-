
#ifndef ATTENDANCE_H
#define ATTENDANCE_H

#include <windows.h>

typedef struct
{
    char employeeID[20];
    char payMonth[10];

    int daysWorked;
    int daysAbsent;

    double overtimeHours;

} Attendance;

void ShowAttendanceForm(HWND hwnd);

#endif