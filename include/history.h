

#ifndef HISTORY_H
#define HISTORY_H

#include <windows.h>

void AddHistoryRecord(
    const char *event,
    const char *employeeID,
    const char *payMonth,
    const char *details
);

void ShowHistoryForm(HWND hwnd);

#endif