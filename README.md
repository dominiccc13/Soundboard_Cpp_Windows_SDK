# Soundboard Usage Guide

## Software Requirements
* Windows OS
* C++17 runtime & Visual C++ Redistributable
* Windows SDK and Standard C++ libraries
* VB-Audio Virtual Cable (VB-Cable)

## Installation

### Option 1
1. You can simply download the raw file: **soundboard.exe**
2. If you do so, you will need to create the following directory: **C:\Users\Public\cpp_soundboard\Resources\Test_Soundbites**
3. You will need to download all 5 soundbites from the **Resources\Test_Soundbites** directory in this repository to that folder. If you do not, you will not be able to play any sounds.

### Option 2
1. Clone this entire repository to **C:\Users\Public**.
2. That's it!

## Program Setup

To enable live feedback:
1. Open **Control Panel** > **Hardware and Sound** > **Sound** > **Recording**.
2. Set your primary recording device to **CABLE Output**.
3. Double-click your new primary recording device and go to the **Listen** tab and check **Listen to this device**.

## Launching
1. Ensure that you have followed installation option 1 or 2 instructions carefully.
1. Double-click **soundboard.exe**. 

To confirm the application is running, check Task Manager or expand the Windows system tray region in your taskbar.

## Controls
Left-click the system tray icon to view available soundbites and their assigned keys.

* **Play Sound:** Ctrl + Shift + Alt + [Key]
* **Exit:** Ctrl + Shift + Alt + Q (Or right-click the system tray icon and click Exit, or end the process in Task Manager.)