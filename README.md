# Gesture Controlled Smart Lamp 💡🖐️

A futuristic, interactive smart lamp controlled entirely by hand gestures. Built using an **Arduino**, **WS2812B LEDs**, and the **APDS9960 Proximity/Gesture Sensor**.

## ✨ Features

* **Touchless Control:** Turn ON/OFF and adjust brightness without touching anything.
* **Auto-Brightness:** Automatically adjusts brightness based on ambient light when first turned on.
* **Manual Brightness Mode:** Hold your hand over the sensor to "unlock" brightness control, then move your hand up/down to adjust.
* **Smart Color Picker:** Enter color selection mode by holding your hand steady.
    * **Breathing Effect:** The lamp "breathes" while you select a color.
    * **5 Custom Colors:** White, Deep Pink, Gold (Reading), Blue-Violet, and Tomato Red.
* **Smooth Fading:** Cinematic fade-in and fade-out effects.

## 🛠️ Hardware Required

* **Microcontroller:** Arduino Nano / Uno / ESP32 (Code is Arduino compatible).
* **LEDs:** WS2812B Addressable LED Strip (e.g., 60 LEDs).
* **Sensor:** SparkFun APDS9960 (Gesture/Proximity/Color).
* **Power Supply:** 5V Power Supply suitable for the number of LEDs used.

## 🔌 Circuit Diagram (Basic)

[ Arduino ] [ APDS9960 ] [ WS2812B Strip ] 5V ----> VCC 5V GND ----> GND GND A4 (SDA) ---> SDA DIN (Pin 6) A5 (SCL) ---> SCL
## 📦 Libraries Used

* **[FastLED](https://github.com/FastLED/FastLED):** For controlling the WS2812B LEDs.
* **[SparkFun_APDS9960](https://github.com/sparkfun/SparkFun_APDS-9960_Sensor_Arduino_Library):** For handling the proximity and gesture sensor.

## 🕹️ How to Use

| Action | Gesture / Trigger |
| :--- | :--- |
| **Turn ON** | Hover hand over sensor (5-25cm). Lamp enters "Auto Mode". |
| **Turn OFF** | While ON, hold hand high (near sensor limit) for 1.2s. |
| **Adjust Brightness** | Hold hand over sensor for 1s to "Unlock". Move hand closer/further to dim/brighten. Remove hand to save. |
| **Change Color** | Hold hand steady at medium height for 3s. The lamp will "breathe". Move hand up/down to scroll colors. Hold still to save. |

## 👨‍💻 Installation

1.  Download the repository.
2.  Open `SmartLamp.ino` in the Arduino IDE.
3.  Install the **FastLED** and **SparkFun_APDS9960** libraries via the Library Manager.
4.  Connect your hardware.
5.  Upload the sketch!
