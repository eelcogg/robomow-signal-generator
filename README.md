# Robomow Signal Generator (ESP32)

## 📖 Overview
**Robomow RX and RT series** robot lawnmowers often show up secondhand for very cheap, but they are often missing the base station that generates the boundary wire signal. Buying original replacement parts for these entry-level models is unfortunately not cost-effective.
This project documents how I reverse engineered the boundary wire signal to build my own base station using an ESP32 and an LM386 audio amplifier.  

I found these same mowers also sold under other brands:

- Robomow RX → also sold as **Wolf Garten Loopo S** and **Cub Cadet XR1**
- Robomow RT → also sold as **Black & Decker BCMW123**  

These models are simple badge-engineered versions with different colors and covers, but internally they are identical. This project applies to all of them.

---

## 🔍 Motivation
I kept noticing used Robomow mowers for sale on my local marketplace. They were cheap, often sold for next to nothing when only needing minor repairs, but nearly all of them were missing the base station that generates the boundary wire signal. These mowers need a boundary wire to nagivate the yard and find their way back to the base station for charging, so without the wire signal generator, the mower is useless.

When I looked into buying replacements, the prices were shocking. A charging head costs around €150, a simple wheel motor €125, and a main board €175. That is more than half the cost of a brand new mower. For the entry-level RX and RT models, it just isn’t economical to buy OEM parts.

That felt wasteful to me. These are solid little machines, and I enjoy fixing things instead of seeing them scrapped. So I connected an oscilloscope to a working system, captured the boundary wire signal, and decided to try reproducing it with hardware I already had on hand.  

Using the ESP32’s DAC output and a simple LM386 amplifier, I managed to inject the signal into the boundary wire. To my surprise, the mower followed the wire perfectly. Even better, by flipping the signal in software, I could create “keep-out” zones inside the mowing area. That was the breakthrough moment when I realized I had a practical, affordable way to bring these abandoned mowers back to life.  

---
## 🔬 Reverse Engineering
To understand how the original Robomow base station worked, I connected a **5 Ω dummy resistor** across the output terminals. With my oscilloscope across the resistor, I observed the following:  

- Signal frequency: a little over **3 kHz**  
- Amplitude: about **2 Vpp** and **868 mVrms**  
- This corresponds to roughly **150–200 mA** flowing in the boundary wire loop  

Here is the original Robomow boundary signal:  

![Original signal](docs/original-signal.png)  

### Signal Processing
Once I captured the waveform, I exported it from the oscilloscope as a **CSV file**. Using a Python script, I:  
1. Detected exactly one period between zero-crossings  
2. Smoothed the waveform with a **Savitzky–Golay filter**  
3. Calculated the RMS value for verification  
4. Plotted the cleaned signal  

Here is the processed waveform:  

![Processed signal](docs/processed-signal.png)  

### Preparing for the ESP32 DAC
To make the signal suitable for the ESP32 DAC, I:  
- Scaled the waveform between **0 and 255**  
- Took **1024 samples** over exactly one period  
- Stored the samples in a **byte array** (`waveform.h`)  

The ESP32 then continuously outputs this array through its DAC, amplified by the LM386, to recreate the boundary wire signal.  

Here is the recreated signal:  

![Recreated signal](docs/recreated-signal.png)  

And here are the two signals overlaid for comparison:  

![Overlay signal](docs/overlay-signal.png)  

The recreated waveform is close enough that the mower behaves exactly as it would with the OEM base station.


## 🛠️ Hardware
- ESP32 development board  
- LM386 audio amplifier module (cheap, widely available)  
- ~7 Ω resistor in series with the boundary wire loop (keeps the LM386 happy with its output impedance)  
- Boundary wire loop (standard garden robot setup)

**Notes on boundary wire:**  
For testing, I used standard 230 V installation wire (2.5 mm²). Any wire works perfectly well for experiments, but for permanent installation in the garden I do **not** recommend the thin wire that often comes with these robots. It is flimsy and easily damaged once buried (ask me how I know 😅). A much better option is to switch to the stronger **shielded boundary wire**, which is more durable and reliable. 


**Setup:**  
ESP32 DAC output → LM386 amplifier → resistor → boundary wire  

**Circuit schematic:**  
![schematic](hardware/schematic.png)  

---

## 💻 Software
The ESP32 code generates the required waveform using the onboard DAC.  

Features:  
- Adjustable frequency and duty cycle  
- Polarity flip in software, which creates keep-out zones inside the mowing area  
- Simple structure, easy to extend  

Source code lives in [`/src`](src/).  

---

## 🧩 Dependencies
This project was developed and tested with the following setup:

- Arduino IDE **2.3.6**  
- ESP32 Board Library **3.3.0** (by Espressif Systems)  
- Core libraries used:  
  - `driver/i2s.h` (part of the ESP32 Arduino core)  
  - `<stdint.h>` and `<string.h>` (standard C libraries, included by default)  

⚠️ Future versions of the ESP32 board library may change APIs.  
If you run into issues, try matching the versions listed above.  

---

## ✅ Results
- Robot successfully follows the generated boundary wire  
- Flipped polarity creates exclusion zones that the robot avoids  
- Tested successfully on multiple secondhand **Robomow RX and RT series** mowers (also Wolf Garten Loopo S and Black & Decker BCMW123)  

---

## 🌱 Where this is going
Right now this project is only a signal generator, but the real goal is to **combine it with a charger** so the system behaves like an original base station:  

- When the mower is out mowing → boundary signal is active  
- When the mower returns to charge → boundary signal switches off, charging voltage switches on  
- When it’s time to mow again → boundary signal switches back on, charger switches off  

This approach saves power and reduces unnecessary EMI, since the boundary signal doesn’t need to radiate 24/7.  

The RX and RT models are fairly “dumb” compared to Robomow’s high-end mowers. They don’t have scheduling or smart features (except Bluetooth on some versions). I am currently also building a **Wi-Fi module** so these mowers can be integrated into **Home Assistant**. That project will live in another repository — stay tuned.  

---

## 🤖 About the Code
Parts of this project’s code were written with the help of ChatGPT.  
I am not a professional programmer, so the implementation may not be perfect.  

I welcome contributions from more experienced developers to improve efficiency, structure, and functionality.  

---

## 🤝 Contributing and Support
Pull requests are very welcome.  
If you want to improve the code, add features, or even port it to other mower brands, go for it.  

Feel free to fork or copy the whole project if it is useful to you. All I ask is a bit of recognition back.  

If you would like to support further reverse engineering of these great little mowers, you can [buy me a coffee](https://www.buymeacoffee.com/YOURNAME). ☕  

---

## 📚 Future Work
- Combine signal generator and charger into one unit  
- Add Wi-Fi or a web interface for dynamic zone configuration  
- Replace LM386 breadboard module with a small dedicated PCB  
- Explore compatibility with other mower brands  

---

## ⚠️ Disclaimer
This is a hobby project. It is not affiliated with, endorsed by, or supported by Robomow, Wolf Garten, or Black & Decker.  
Use at your own risk. Not suitable for commercial or safety-critical use.  

---

## 📜 License
MIT License – do whatever you like, just give credit and do not blame me if your mower goes rogue.  

If you would like to support further r
