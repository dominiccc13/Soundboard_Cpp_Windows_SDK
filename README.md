# Soundboard Usage Guide

## Software Requirements
* Windows OS
* C++17 runtime & Visual C++ Redistributable
* Windows SDK and Standard C++ libraries
* VB-Audio Virtual Cable (VB-Cable)

## Installation
1. Download and install VB-Cable from [VB-Audio](https://vb-audio.com/Cable/index.htm)
    * _This application has only been tested with the new 64 bit package._
2. Download soundboard.zip from the GitHub release in this repository.
3. Unzip soundboard.zip using 7Zip or another tool.
4. This will extract to the **dist** directory in the directory you unzipped in.
5. Open **dist** and double-click **soundboard.exe**.

To confirm the application is running, check Task Manager or expand the Windows system tray region in your taskbar.

## Program Setup
To enable live feedback:
1. Open **Control Panel** > **Hardware and Sound** > **Sound** > **Recording**.
2. Set your primary recording device to **CABLE Output**.
3. Double-click your new primary recording device and go to the **Listen** tab and check **Listen to this device**.

## Controls
Double-click the system tray icon to open the soundbite window. It lists every soundbite with its path and hotkey.

* **Add:** Choose a .wav or .mp3 file and a hotkey for the new soundbite.
* **Edit:** Select a soundbite and change its path or hotkey.
* **Remove:** Select a soundbite and remove it. The other soundbites keep their hotkeys.

Keys are assigned per soundbite and can be any number or letter except P.

* **Play Sound:** Ctrl + Shift + Alt + [Key]
* **Exit:** Ctrl + Shift + Alt + P (Or right-click the system tray icon and click Exit, or end the process in Task Manager.)