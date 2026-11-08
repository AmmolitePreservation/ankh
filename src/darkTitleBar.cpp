#include "darkTitleBar.h"

#include <QWidget>

#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>

void applyDarkTitleBar(QWidget* window)
{
  const BOOL isDark = TRUE;
  DwmSetWindowAttribute(reinterpret_cast<HWND>(window->winId()), DWMWA_USE_IMMERSIVE_DARK_MODE, &isDark, sizeof(isDark));
}
