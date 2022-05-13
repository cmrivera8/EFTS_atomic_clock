# -*- coding: utf-8 -*-

"""package mac
author    Benoit Dubois
copyright FEMTO ST, 2021
license   GPL v3.0+
brief     MAC device drivers.
"""

import time
import logging
import serial
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

    _current_word = 23657  # 0.92mA environ pour vixar

    def startup(self):
        self.parent.send("0")

    @property
    def current(self):
        return self._current_word / (2.**16) * 2.5

    @current.setter
    def current(self, value):
        assert 0 <= value < 2
        self._current_word = int(value/2.5*(2.**16))
        self.parent.send("9 " + str(self._current_word))


# ============================================================================
class MagFieldCurrentSource(SubComponent):

    _current_word = 10000  # 1.7 mA

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

    def lock(self):
        logging.info("Lock ON")
        self.send("3")


# ============================================================================
if __name__=='__main__':
    mac = Mac()
