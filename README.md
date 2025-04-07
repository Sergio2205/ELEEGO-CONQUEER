# 🤖 ELEGOO Conqueror Robot Tank Kit (2024)

This project is based on the **ELEGOO Conqueror Robot Tank Kit**, a smart tracked robot designed for Arduino-based learning and robotics projects. It features obstacle avoidance, infrared remote control, real-time tracking, and app-based control via Bluetooth.

This repository includes the code and resources needed to program and operate the robot, along with detailed explanations of its functions and capabilities.

---

## 📦 Features

The robot is equipped with multiple functionalities:

### 1. Obstacle Avoidance Mode (`AvoidMode()`)
Uses the ultrasonic sensor to detect obstacles and navigate around them autonomously. When an obstacle is detected, the robot calculates an alternative direction to avoid a collision.

### 2. IR Remote Control Mode (`IR_Control()`)
Allows control using the included IR remote. Each button corresponds to a specific movement or action (forward, backward, turn left/right, stop, speed change, etc.).

### 3. Line Tracking Mode (`TrackingMode()`)
Uses infrared sensors on the bottom of the robot to follow a black line on a white surface. Ideal for line-following competitions or path-based navigation.

### 4. Bluetooth Control Mode (`Bluetooth_Control()`)
Enables control via a smartphone app using Bluetooth communication. The app sends characters or strings that the robot interprets to move in different directions or perform actions.

### 5. Servo Radar Scan (`ServoRadarScan()`)
A servo motor moves the ultrasonic sensor in a sweeping motion to scan for obstacles in the surrounding area, allowing better decisions in obstacle avoidance.

### 6. Speed and Direction Control (`Set_Speed()`, `Advance()`, `Back()`, `TurnLeft()`, `TurnRight()`, `Stop()`)
Basic motor control functions for movement and speed. Used by all driving modes to perform precise motions.

---

## 🧠 Programming Overview

The robot is programmed using **Arduino C++** and runs on an **Arduino Uno-compatible board**. The code uses the following components:

- **Ultrasonic sensor** (HC-SR04) for distance detection
- **IR sensors** for line tracking
- **IR receiver** for remote control
- **Bluetooth module (HC-06)** for smartphone connectivity
- **Servo motor** for radar scanning
- **Motor driver (L298N)** to control the two motors

The main structure uses a `loop()` that checks the selected mode and executes the corresponding behavior.

### 🔧 Libraries Used

- `IRremote.h` – for reading signals from the IR remote
- `SoftwareSerial.h` – for Bluetooth communication
- `Servo.h` – for servo motor control
- Custom utility and hardware abstraction functions

---

## 🚀 How to Upload & Run

1. **Install the Arduino IDE**
   - Download from: [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)

2. **Install Required Libraries**
   - IRremote, Servo, SoftwareSerial (via Library Manager)

3. **Upload the Code**
   - Connect your board via USB
   - Select the correct COM port and board type in the Arduino IDE
   - Open the main `.ino` file from this repository
   - Click `Upload`

4. **Power the Robot**
   - Use a 7.4V Li-ion battery pack or USB power for testing

---

## 📱 Mobile App Control

To control via Bluetooth:

1. Pair your phone with the HC-06 module (default password: `1234`).
2. Use any Bluetooth terminal app or the ELEGOO app.
3. Send control characters like:
   - `F` = Forward
   - `B` = Backward
   - `L` = Left
   - `R` = Right
   - `S` = Stop

---

## 📸 Gallery

*(Include photos or a video link showing the robot in action)*

---

## 🛠️ Troubleshooting

- **Robot not responding to IR remote**: Check that the IR receiver is wired properly and the remote has working batteries.
- **Bluetooth not pairing**: Ensure HC-06 module is powered and not already connected to another device.
- **Robot not avoiding obstacles**: Check ultrasonic wiring and distance measurement logic.

---

## 🙌 Credits

- **ELEGOO Inc.** – Original hardware design and base code
- Community contributors and makers

---

## 📄 License

This project is under the MIT License – free to use and modify for educational purposes.

