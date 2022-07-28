# -*- coding: utf-8 -*-

"""package mac
author    Benoit Dubois (updated by Carlos RIVERA, july 2022)
copyright FEMTO ST, 2021
license   GPL v3.0+
brief     MAC device drivers.
"""

import time
import logging
import serial
import misc
#import subcomponent as sc
from adf4158 import Adf4158


# ============================================================================
class SubComponent():
    def __init__(self, parent):
        self.parent = parent


# ============================================================================
class ScAdf4158(Adf4158, SubComponent):

    def __init__(self, parent, ref_in_freq=10e6):
        SubComponent.__init__(self, parent)
        super().__init__(ref_in_freq=ref_in_freq)

    def send_reg(self, reg):
        self.parent.send("4 " + str(reg))


# ============================================================================
class LaserCurrentSource(SubComponent):
    _current_word = 23657  # 23657 = 0.92 mA

    def startup(self):
        self.parent.send("2")

    def reset(self):
        self.parent.send("9 " + str(self._current_word))

    @property
    def current(self):
        return (self._current_word / (2.**16)) * 2.5

    @current.setter
    def current(self, value):
        assert 0 <= value < 1.7
        self._current_word = int((value/2.5)*2.**16)
        self.parent.send("9 " + str(self._current_word))


# ============================================================================
class MagFieldCurrentSource(SubComponent):

    _current_word = 0 # 0 = 0 mA and 10000 = 1.7 mA

    @property
    def current(self):
        return self._current_word / (2.**16) * 50.

    @current.setter
    def current(self, value):
        assert 0 <= value < 50
        self._current_word = int(value / 50. * (2.**16))
        logging.debug(self._current_word)
        self.parent.send("8 " + str(self._current_word))


# ============================================================================
class LaserTECcontroller(SubComponent):

    _tec_volt_word = 8800
    _tec_state = False

    @property
    def tec_state(self):
        return self._tec_state

    @tec_state.setter
    def tec_state(self, state):
        _tec_state = bool(state)
        if _tec_state:
            logging.info("TEC On")
            self.parent.send("6")
        else:
            logging.info("TEC Off")
            self.parent.send("7")

    @property
    def tec_volt_word(self):
        return self._tec_volt_word

    @tec_volt_word.setter
    def tec_volt_word(self, value):
        assert 1000 <= value <= 9800
        self._tec_volt_word = int(value)
        logging.debug(self._tec_volt_word)
        self.parent.send("5 " + str(self._tec_volt_word))

    @property
    def tec_resistance(self):
        return 10000.*(1./float(2**14/float(self._tec_volt_word)-1))

    @tec_resistance.setter
    def tec_resistance(self, resistance):
        self.tec_volt_word = int(2**14/(10000./float(resistance)+1))


# ============================================================================
class Mac():

    def __init__(self):
        self._waiting = False
        self.ser = serial.Serial(baudrate=115200, timeout=1)
        self.adf = ScAdf4158(self)
        self.lascursour = LaserCurrentSource(self)
        self.magcursour = MagFieldCurrentSource(self)
        self.lastecctrl = LaserTECcontroller(self)
        # Carlos RIVERA:
        # Group 1: Laser and magnetic field control
        # Sub-group 1: Laser scan
        self.start_current= misc.current_to_word(1.1)#28000 # 1.0681 mA
        self.end_current= misc.current_to_word(1.2) #34000 # 1.2970 mA
        self.number_samples=300
        # Group 2: Laser lock
        self.laser_lock_initial_value=misc.current_to_word(1.298) #31457 #1.2 mA
        self.laser_mod_width=misc.current_to_word(114.441e-6) # 38.147 nA = 1 machine unit
        self.laser_kp=0.07
        # Group 3: Quartz lock
        # Sub-group 1: Error signal plot
        self.start_quartz=misc.frequency_to_word(0)
        self.end_quartz=misc.frequency_to_word(60000)
        # Sub-group 2: CPT lock
        self.quartz_lock_initial_value=misc.frequency_to_word(50000)
        self.quartz_frequency_offset=0
        self.quartz_kp=0.4
        self.quartz_ki=0.01
        self.quartz_kd=0
    def connect(self, port):
        self.ser.port = port
        if not self.ser.is_open:
            self.ser.open()
        logging.info("MAC connection succeed")

    def disconnect(self):
        if self.ser.is_open:
            self.ser.close()
        logging.info("MAC disconnection succeed")

    def __del__(self):
        self.disconnect()

    @property
    def waiting(self):
        return self._waiting

    def readline(self):
        if self.ser is not None:
            self.ser.readline().decode("utf-8")
        else:
            logging.error("Serial device not connected")

    def read(self, *argv, **kwargs):
        if self.ser is not None:
            self.ser.read(*argv, **kwargs).decode("utf-8")
        else:
            logging.error("Serial device not connected")

    def send(self, command):
        if self.ser is not None:
            self.ser.write((command + "\r\n").encode("utf-8"))
            logging.debug("Send %r", command)
        else:
            logging.error("Serial device not connected")

    def query(self, command):
        while self._waiting:
            pass
        self._waiting = True
        self.send(command)
        time.sleep(0.020)
        response = self.ser.readline()[:-2].decode("utf-8")
        self._waiting = False
        return response

    def startup(self):
        logging.info("Start up sequence")
        self.send("2")

    def start_ramp(self):
        logging.info("Start ramp of the laser")
        command="t {} {} {} s".format(self.start_current,self.end_current,self.number_samples)
        self.send(command)
    
    def lock_laser(self):
        logging.info("Start lock of the laser")
        self.update_pid()
        self.update_laser_parameters()
        time.sleep(0.25)
        command = "l"
        self.send(command)
    
    def lock_quartz(self):
        logging.info("Plotting error signal")
        self.update_pid()
        self.update_laser_parameters()
        self.update_quartz_scan_parameters()
        self.update_quartz_parameters()
        command = "b"
        self.send(command)

    def lock_cpt(self):
        logging.info("Start lock of the quartz")
        self.update_pid()
        self.update_laser_parameters()
        self.update_quartz_scan_parameters()
        self.update_quartz_parameters()
        command = "0"
        self.send(command)
        
    def update_laser_parameters(self):
        command = "i {} {}".format(self.laser_lock_initial_value,self.laser_mod_width)
        self.send(command)

    def update_quartz_parameters(self):
        command = "j {} {}".format(int(self.quartz_lock_initial_value), int(self.quartz_frequency_offset))
        self.send(command)

    def update_quartz_scan_parameters(self):
        command = "v {} {}".format(int(self.start_quartz),int(self.end_quartz))
        self.send(command)

    def update_pid(self):
        command = "a {} {} {} {}".format(self.quartz_kp,self.quartz_ki,self.quartz_kd,self.laser_kp)
        self.send(command)

    def lock(self):
        logging.info("Lock ON")
        self.send("3")

    def stop_signal(self):
        self.send("x")

# ============================================================================
if __name__=='__main__':
    mac = Mac()
