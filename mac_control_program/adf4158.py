# -*- coding: utf-8 -*-

import logging

RAMP_MODE = {"CONTINUOUS_SAWTOOTH" : 0b00,
	     "CONTINUOUS_TRIANGULAR" : 0b01,
	     "SINGLE_SAWTOOTH" : 0b10,
	     "SINGLE_RAMP_BURST" : 0b11}

MUX_OUT_CONTROL = {"READBACK_TO_MUXOUT": 0b1111}


## class Adf4158(sc.SubComponent):
class Adf4158():

    # R0
    _ramp_on = 1               # 1 bit
    _mux_out_control = MUX_OUT_CONTROL["READBACK_TO_MUXOUT"] # 4 bits
    _integer_value = 459       # 12 bits
    _msb_frac_value = 2581     # 12 bits
    # R1
    _lsb_frac_value = 592      # 13 bits (DBB)
    # R2
    _csr_en = 0                # 1 bit
    _cp_current_set = 0        # 4 bits    0 = 0.3125 mA
    _prescaler = 1             # 1 bit (1 = 8/9)
    _rdiv2 = 0                 # 1 bit (R Divider)
    _reference_doubler = 0     # 1 bit (reference doubler)
    _r_counter = 1             # 5 bit (r-counter divide ratio)
    _clk1_divider = 1          # 12 bits (Clk1 divider value)
    # R3
    _n_sel = 0
    _sd_reset = 0
    _ramp_mode = RAMP_MODE["CONTINUOUS_SAWTOOTH"]
    _psk_enable = 0
    _fsk_enable = 0
    _ldp = 0
    _pd_polarity = 1
    _power_down = 0
    _cp_three_state = 0
    _counter_reset = 0
    # R4
    _le_sel = 0
    _sigma_delta_modulator_mode = 0
    _neg_bleed_current = 0
    _readback_muxout = 3
    _clk_div_mode = 3
    _clk2_divider_value = 100
    # R5
    _tx_ramp_clk = 0
    _par_ramp = 0              # Parabolic ramp
    _interrupt = 0
    _fsk_ramp_en = 0
    _ramp_2_en = 0
    _up_dev_sel = 0            # RAMP1
    _up_dev_offset_word = 2    # Up ramp DEVoff
    _up_deviation_word = 84    # Up ramp DEV
    _down_dev_sel = 1          # RAMP2
    _down_dev_offset_word = 0  # Down ramp DEVoff
    _down_deviation_word = 0   # Down ramp DEV
    # R6
    _up_step_word = 2000
    _down_step_word = 0
    # R7
    _ramp_del_fl = 0
    _ramp_del = 0
    _del_clk_sel = 0
    _del_start_en = 0

    def __init__(self, ref_in_freq=10e6):
        self._ref_in_freq = ref_in_freq  # Reference In frequency Hz

    def send_all_reg(self):
        for reg in [self.R7(), self.R6_1(), self.R6_2(), self.R5_1(), self.R5_2(),
                    self.R4(), self.R3(), self.R2(), self.R1(), self.R0()]:
            self.send_reg(reg)

    def R0(self):
        return (self._ramp_on << 31) + (self._mux_out_control << 27) \
            + (self._integer_value << 15) + (self._msb_frac_value << 3) + 0

    def R1(self):
        return (self._lsb_frac_value << 15) + 1

    def R2(self):
        return (self._csr_en << 28) + (self._cp_current_set << 24) \
            + (self._prescaler << 22) + (self._rdiv2 << 21) \
            + (self._reference_doubler << 20) + (self._r_counter << 15) \
            + (self._clk1_divider << 3) + 2

    def R3(self):
        return (self._n_sel << 15) + (self._sd_reset << 14) \
            + (self._ramp_mode << 10) + (self._psk_enable << 9) \
            + (self._fsk_enable << 8) + (self._ldp << 7) \
            + (self._pd_polarity << 6) + (self._power_down << 5) \
            + (self._cp_three_state << 4) + (self._counter_reset << 7) + 3

    def R4(self):
        return (self._le_sel << 31) + (self._sigma_delta_modulator_mode << 26) \
            + (self._neg_bleed_current << 23) + (self._readback_muxout << 21) \
            + (self._clk_div_mode << 19) + (self._clk2_divider_value << 7) + 4

    def R5_1(self):
        return (self._tx_ramp_clk << 29) + (self._par_ramp << 28) \
            + (self._interrupt << 26) + (self._fsk_ramp_en << 25) \
            + (self._ramp_2_en << 24) + (self._up_dev_sel << 23) \
            + (self._up_dev_offset_word << 19) + (self._up_deviation_word << 3) + 5

    def R5_2(self):
        return (self._tx_ramp_clk << 29) + (self._par_ramp << 28) \
            + (self._interrupt << 26) + (self._fsk_ramp_en << 25) \
            + (self._ramp_2_en << 24) + (self._down_dev_sel << 23) \
            + (self._down_dev_offset_word << 19) \
            + (self._down_deviation_word << 3) + 5

    def R6_1(self):
        return (self._up_dev_sel << 23) + (self._up_step_word << 3) + 6

    def R6_2(self):
        return (self._down_dev_sel << 23) + (self._down_step_word << 3) + 6

    def R7(self):
        return (self._ramp_del_fl << 18) + (self._ramp_del << 17) \
            + (self._del_clk_sel << 16) + (self._del_start_en << 15) + 7

    @property
    def ref_in_freq(self):
        return self._ref_in_freq

    @property
    def ramp_on(self):
        return self._ramp_on

    @ramp_on.setter
    def ramp_on(self, value):
        self._ramp_on = int(bool(value))
        logging.debug("R0: %r", hex(self.R0()))
        self.send_reg(self.R0())

    @property
    def clk2_divider_value(self):
        return self._clk2_divider_value

    @clk2_divider_value.setter
    def clk2_divider_value(self, value):
        assert 0 <= value <= 4095
        self._clk2_divider_value = int(value)
        logging.debug("R4: %r", hex(self.R4()))
        logging.debug("R0: %r", hex(self.R0()))
        self.send_reg(self.R4())
        self.send_reg(self.R0())

    @property
    def _freq_pfd(self):
        """
        returns: Phase Frequency Detector frquency (float).
        """
        pfd_0 = self._ref_in_freq * (
            (1 + self._reference_doubler) \
            / float(self._r_counter * (1 + self._rdiv2)) )
        pfd_1 = self._ref_in_freq \
            * (1 + self._reference_doubler) \
            / self._r_counter \
            / (1 + self._rdiv2)
        return pfd_0

    @property
    def _frac_value(self):
        """
        :returns: Fractional value (float).
        """
        return ((self._msb_frac_value << 13) + self._lsb_frac_value) / 2.**25

    @property
    def frequency(self):
        return (self._integer_value + self._frac_value) * self._freq_pfd

    @frequency.setter
    def frequency(self, out_freq):
        """Set output frequency of synthesizer.
        :param out_freq: output frequency value (float)
        :returns: None
        """
        n = int(out_freq / self._freq_pfd)
        assert 23 <= n < 4095
        f_reg = ((out_freq / self._freq_pfd) - n) * 2**12
        f_msb = int(f_reg)
        assert 0 <= f_msb < 4095
        f_lsb = int((f_reg - f_msb) * 2**13)
        assert 0 <= f_lsb < 8191
        self._integer_value = n
        self._msb_frac_value = f_msb
        self._lsb_frac_value = f_lsb
        logging.debug("R1 %r", hex(self.R1()))
        logging.debug("R0 %r", hex(self.R0()))
        self.send_reg(self.R1())
        self.send_reg(self.R0())

    @property
    def freq_dev(self):
        """
        :returns: Frequency deviation per step (float)
        """
        return self._freq_pfd / 2**25 \
                    * float(self._up_deviation_word) \
                    * 2**float(self._up_dev_offset_word)

    @freq_dev.setter
    def freq_dev(self, freq):
        up_dev_word = int(freq / (self._freq_pfd / (2**25) * (2**float(self._up_dev_offset_word))))
        assert -32768 <= up_dev_word <= 32767
        self._up_deviation_word = up_dev_word
        logging.debug("R5_1 %r", hex(self.R5_1()))
        logging.debug("R0 %r", hex(self.R0()))
        self.send_reg(self.R5_1())
        self.send_reg(self.R0())

    @property
    def total_freq_ramp(self):
        return self.freq_dev * self._up_step_word

    @total_freq_ramp.setter
    def total_freq_ramp(self, freq):
    	self.freq_dev = freq / float(self._up_step_word)

    @property
    def time_per_step(self):
        """
        :returns: Time between each step (float)
        """
        return self._clk1_divider * self._clk2_divider_value / float(self._freq_pfd)

    @property
    def total_time_ramp(self):
        return self.time_per_step * self._up_step_word

    @property
    def chan_spacing(self):
        return 1 / float(2**25) * self._freq_pfd

    @property
    def ramp_steps(self):
        return self._up_step_word

    @ramp_steps.setter
    def ramp_steps(self, steps):
        assert 0 <= steps <= 1048575
        self._up_step_word = int(steps)
        logging.debug("R6_1 %r", hex(self.R6_1()))
        logging.debug("R0 %r", hex(self.R0()))
        self.send_reg(self.R6_1())
        self.send_reg(self.R0())

    def preview_all_registers(self):
    	print("""R0:{:08x} R1:{:08x} R2:{:08x} R3:{:08x} R4:{:08x}\n\
        R5_1:{:08x} R5_2:{:08x} R6_1:{:08x} R6_2:{:08x}\n\
        R7:{:08x}""".format(self.R0(), self.R1(), self.R2(), self.R3(),
                            self.R4(), self.R5_1(), self.R5_2(), self.R6_1(),
                            self.R6_2(), self.R7()))

