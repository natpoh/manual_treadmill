# VR Treadmill Controller

A high-performance C++ utility with a Direct3D 11 + Dear ImGui interface designed to read serial data from a VR manual treadmill sensor and emulate virtual Xbox 360 controller inputs.

This project translates physical walking speed (detected via magnetic reed switch sensors connected to an Arduino) into analog joystick inputs on a virtual gamepad.

---

## Features

- **Cascaded Double Simple Moving Average (SMA)**:
  - **SMA 1**: Counts the number of active switch trigger events within a sliding window (e.g. 3000ms).
  - **SMA 2**: Performs secondary smoothing over the calculated speed values (e.g. 500ms) to ensure smooth transitions without discrete speed steps.
- **100Hz Simulation Physics Loop**: Runs a fixed-interval `10ms` simulation tick to guarantee stable movement decay and response.
- **Dynamic Speed Clamping & Thresholds**:
  - A configurable **Maximum Speed** target slider controls the speed divisor (e.g., reaching 50 pulses in the window triggers 100% gamepad forward deflection).
  - Gamepad output activates dynamically (binary trigger) when the smoothed SMA 2 speed crosses the configurable Maximum Speed threshold line.
- **Double-Graph Real-time Oscilloscope**:
  - **Sensor Analog Signal (Top)**: Plots the raw analog values (`0 - 1023`) from pin `A0` along with the Trigger Threshold.
  - **Calculated Speed (Bottom)**: Displays the raw pulse counts (SMA 1, in blue), the smoothed velocity curve (SMA 2, in green), and the Maximum Speed target threshold (red line).
  - Displays a clean, smooth, rolling 5-second timeline without X-axis tick labels for a clean aesthetic.

---

## Hardware Setup

1. **Arduino Sensor**: Connect a magnetic reed switch or hall-effect sensor to pin `A0` of your Arduino board.
2. **Firmware**: Upload the code in the `arduino_sensor` folder to the board. The sensor reads values and sends them to the PC via a USB Serial Port at 200 Hz.

---

## Requirements & Software Setup

1. **Virtual Gamepad Driver**: Install the [ViGEmBus](https://github.com/ViGEm/ViGEmBus) driver on your system. This allows the application to create a virtual Xbox 360 controller.
2. **Windows OS**: The GUI is optimized for Windows 10/11 using MSVC compiler toolchain.

---

## Compilation

Build the project from the `cpp_app` subdirectory:

1. Open the Developer Command Prompt for Visual Studio.
2. Navigate to the `cpp_app` directory.
3. Run the automated build script:
   ```powershell
   cmd /c build.bat
   ```
4. The compiled executable `VRTreadmill.exe` will be located in the `cpp_app/build/` directory.

---

## GUI Parameters

- **Trigger Threshold**: Configures the signal deflection delta from the center baseline (1023) required to count a sensor trigger.
- **SMA 1 Period (ms)**: The window size used to count sensor triggers (pulses).
- **SMA 2 Period (ms)**: The window size used to smooth out speed transitions.
- **Maximum Speed**: The pulse count target that triggers a 100% forward joystick output.
