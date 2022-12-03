hackquad_firmware
-----------------
The firmware for the hackquad esp32-based quadcopter.

### Build Instructions
1. Install IDF
2. Ensure you have dialout permissions `sudo usermod -a -G dialout $USER; sudo reboot`
3. Run `idf.py flash -b 921600` in a terminal initialized with IDF ENV.
