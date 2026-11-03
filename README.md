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
Double-click the system tray icon to open the soundbite window. It lists every soundbite with its path and hotkey.

* **Add:** Choose a .wav file and a hotkey for a new soundbite (up to 20).
* **Edit:** Select a soundbite and change its path or hotkey.
* **Remove:** Select a soundbite and remove it. The other soundbites keep their hotkeys.

Keys are assigned per soundbite and can be any number or letter except P.

* **Play Sound:** Ctrl + Shift + Alt + [Key]
* **Exit:** Ctrl + Shift + Alt + P (Or right-click the system tray icon and click Exit, or end the process in Task Manager.)