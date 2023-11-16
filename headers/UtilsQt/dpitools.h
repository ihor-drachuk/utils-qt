/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once

// Warning! Also see `qt.conf` file with additional
// settings for DPI/scaling handling.
//   0 - DPI Unaware
//   1 - System-DPI Aware
//   2 - Per-Monitor DPI Aware (no system scaling)

class QCoreApplication;

class DpiTools
{
public:
    static void setAutoScreenScale(bool value);
    static void setScaleFactor(float value);
    static void ignoreSystemAutoResize(QCoreApplication& app);
};
