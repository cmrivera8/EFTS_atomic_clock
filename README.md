# EFTS_atomic_clock
The atomic clock described in this repository is used in the yearly European Frequency and Time Seminar (EFTS) hosted at ENSMM. This repository contains the original design files as well as the improved control software GUI in python (EFTS_atomic_clock/mac_control_program/qt_mac.py) and the microcontroller code (EFTS_atomic_clock/mac/mac.ino) embedded in the demonstrator.

When I started my contribution on this project, there were three atomic clocks, out of which only one was working correctly. The other two were marginally operational, with a weak absorption signal and low signal-to-noise ratio. This made it very difficult to lock the servo loops on the desired electronic transition. Moreover, the GUI was not intuitive and resulted in a not-ideal learning experience for the seminar attendees.

![alt text](https://github.com/cmrivera8/EFTS_atomic_clock/blob/9414f2e2d7a4e448c92909df148454d10a288dab/Screen%20captures/GUI%20-%20Readme%20file.jpg?raw=true)
