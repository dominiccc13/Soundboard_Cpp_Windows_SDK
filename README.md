# Soundboard Usage Guide

## Software Requirements
* Windows OS
* C++17 runtime & Visual C++ Redistributable
* Windows SDK and Standard C++ libraries
* VB-Audio Virtual Cable (VB-Cable)

## Program Setup
To enable live feedback:
1. Open **Control Panel** > **Hardware and Sound** > **Sound** > **Recording**.
2. Set your primary recording device to **CABLE Output**.
3. Double-click your new primary recording device and go to the **Listen** tab and check **Listen to this device**.

## Launching
1. You made need to adjust soundbiteBasePath in line 1 of the main() function in Main.cpp.
2. Open the Build folder.
3. Double-click soundboard.exe.

To confirm the application is running, check Task Manager or expand the Windows system tray region in your taskbar.

## Controls
Left-click the system tray icon to view available soundbites and their assigned keys.

* **Play Sound:** Ctrl + Shift + Alt + [Key]
* **Exit:** Ctrl + Shift + Alt + Q (Or right-click the system tray icon and click Exit, or end the process in Task Manager.)