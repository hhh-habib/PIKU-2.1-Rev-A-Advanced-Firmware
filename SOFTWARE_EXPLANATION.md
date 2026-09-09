# PIKU 2.1 Rev A — Software Explanation

This is a simple study guide for the validated standalone PIKU 2.1 Rev A firmware. For engineering details, configuration values, and test evidence, see the [main README](README.md).

## What this software is

PIKU 2.1 Rev A is the embedded brain software of a mobile environmental inspection robot. It runs on an ESP32-S3 and brings the robot's movement, sensors, autonomous navigation, environmental monitoring, communication, and safety behavior together.

This repository contains the standalone robot firmware. It is not the later Stage 4 CPS Integration repository and is not the complete multi-node CPS platform.

## What the robot can do

- Drive under Manual control
- Navigate in AUTO mode
- Detect obstacles
- Scan left, front, and right
- Choose a safer path
- Recover when its path is blocked
- Detect configured environmental hazards
- Stop when a hazard is active
- Resume AUTO after a continuously safe clear period
- Provide live information through a browser dashboard
- Work through direct SoftAP Wi-Fi or STA/Home Wi-Fi

## Main hardware

- **ESP32-S3:** runs the firmware and Wi-Fi dashboard.
- **Four-wheel-drive chassis:** provides the mobile base.
- **L298N:** controls the drive motors.
- **SG90 servo:** turns the ultrasonic sensor to scan different directions.
- **HC-SR04:** checks the front and scanned areas with ultrasonic ranging.
- **Left and right IR sensors:** monitor low obstacle zones near the front of the chassis.
- **MPU6050:** measures motion and short-term rotation.
- **DHT22:** measures temperature and humidity.
- **MQ-2:** provides a relative gas/smoke sensor signal.
- **Digital flame sensor:** provides an optical flame/infrared hazard signal.
- **OLED:** shows local status and warnings.
- **nRF24:** provides a secondary experimental radio link.

## How obstacle avoidance works

When AUTO detects an obstacle, the normal recovery sequence is:

    Obstacle detected
      → STOP
      → reverse slightly
      → scan LEFT / FRONT / RIGHT
      → choose a safe side
      → spin approximately 35°
      → check the sensors again
      → continue AUTO when safe

The left IR sensor is connected to GPIO7, and the right IR sensor is connected to GPIO8. They monitor low obstacle zones near the robot body. The servo-mounted HC-SR04 monitors a different, higher scanning zone. These sensors complement one another; one clear sensor does not automatically cancel an obstacle reported by another valid sensor.

Reflective IR sensing depends on surface color, angle, distance, and lighting. In physical tests, representative black objects were not reliably detected, so the ultrasonic sensor and supervised operation remain important.

## MPU6050

The MPU6050 is used for:

- motion sensing;
- tilt and movement information; and
- short-term relative gyro rotation.

During an AUTO turn, gyro measurements help control an approximately 35° relative spin. This is not a compass, absolute heading, localization system, or wheel odometry.

## Environmental monitoring

- **DHT22:** reports temperature and humidity.
- **MQ-2:** reports a relative gas/smoke signal. It is not calibrated in ppm.
- **Flame DO sensor:** reports an optical flame/infrared hazard signal through its digital output.

The final tested flame-sensor setup included a small black optical shroud to reduce unwanted light. This remains prototype hazard sensing, not certified fire detection.

## Hazard behavior

When a configured environmental hazard is detected:

    Hazard detected
      → robot stops
      → dashboard shows the alarm
      → robot keeps monitoring

When the hazard clears:

    Hazard clears
      → remain continuously clear for approximately 2 seconds
      → scan the surroundings
      → continue AUTO when safe

If the hazard returns during the clear period, the alarm returns, the timer resets, and the robot remains stopped.

## Communication

Wi-Fi is the primary control and telemetry path. It can operate in two ways:

- **SoftAP:** a phone or computer connects directly to the robot.
- **STA/Home Wi-Fi:** the robot joins a provisioned local 2.4 GHz network.

The nRF24 is a secondary, experimental machine-to-machine communication path. It is not the primary interactive control system.

## Dashboard

The browser dashboard provides:

- sensor information;
- Manual and AUTO mode selection;
- movement controls;
- radar and scan information;
- communication status; and
- safety and system diagnostics.

## Important source files

- **MotorController:** controls motor direction and speed.
- **NavigationManager:** runs AUTO navigation, scanning, turning, retry, and recovery states.
- **SensorManager:** reads sensors and prepares consistent sensor snapshots.
- **SafetyManager:** decides when hazards or sensor conditions must stop movement.
- **WebDashboard:** manages Wi-Fi, browser requests, commands, and dashboard data.
- **DashboardPublisher:** sends updated telemetry at controlled rates.
- **RadioManager:** manages the bounded secondary nRF24 communication path.
- **ImuManager:** samples the MPU6050 and reports its health and motion data.
- **ServoScanner:** moves the scanning servo to left, front, and right positions.
- **main.cpp:** starts the subsystems and coordinates the priority service loop and robot modes.

## Physical testing summary

The final prototype test observations included:

- an approximately 30-minute final test;
- successful physical operation through both SoftAP and STA Wi-Fi;
- practical STA operation around 30 ft / 9 m in the tested environment;
- AUTO spins observed at approximately 30–35°;
- representative obstacles detected by the reflective IR sensors around approximately 7 cm;
- tested black objects not reliably detected by reflective IR;
- no false hazard alarms observed during the final approximately 30-minute test with the flame-sensor shroud installed;
- working flame, gas, and temperature paths; and
- qualitatively responsive control and telemetry.

These are observations from a functional research prototype in its tested environment. They are not calibrated specifications, formal latency measurements, safety certification, or guaranteed performance.

## Where this repository fits

This repository makes PIKU 2.1 Rev A work as a validated standalone robot. The separate Stage 4 CPS Integration stage later migrates this foundation into a multi-node architecture involving CPS Node V1, PIKU 2.0.1, and PIKU 2.1. Capabilities such as localization, mapping, ROS 2, edge intelligence, and more advanced coordination remain future research directions rather than features of this standalone firmware.

## In simple words

PIKU 2.1 Rev A is the brain software of an autonomous environmental inspection robot. It allows the ESP32-S3 robot to sense its surroundings, avoid obstacles, monitor hazards, move automatically, receive Wi-Fi commands, and report its condition through a browser dashboard.
