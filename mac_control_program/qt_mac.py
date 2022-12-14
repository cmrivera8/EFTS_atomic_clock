#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""package mac
author    J. Rutkowski, update by Benoit Dubois (add Linux compatibility)
          2022: updated by Carlos RIVERA:
          - Added visual controls to operate the demonstrator without console commands.
          - Converted all machine units in the GUI (graph and controls) to physical units.
          - Completely redesigned the "Control panel".
          - Added the option to change the laser modulation width.
          - Added the option to change the Quartz scan parameters (initial and final values).
          - Added double function buttons to start/processes without relying on the physical button.
          - Added Absorption Signal graph to monitor the output of the photodiode and not use an oscilloscope.
          - Added titles and axis labels to all the graphs.
          - Added "Maximum Absorption" graphic tool.
          - Added servo loops lock state markers for Laser and Quartz graphs
          - Added CPT signal graph
copyright FEMTO ST, 2022
license   GPL v3.0+
brief     Main script of the MAC controller graphical interface.
"""

import sys
import time
import logging
import signal
import argparse
from serial.tools.list_ports import comports
import pyqtgraph as pg
from pyqtgraph.dockarea import *
from pyqtgraph.Qt import QtGui, QtCore
import pyqtgraph.parametertree.parameterTypes as pTypes
from pyqtgraph.parametertree import Parameter, ParameterTree
##from ui.pyqtgraphaddon import MyFloatParameter
import mac_device as md
import misc
import numpy as np
# ============================================================================
class SerialReader(pg.QtCore.QThread):

    new_value = pg.QtCore.Signal(str)
    current_buffer = misc.Buffer(1000)
    quartzword_buffer = misc.Buffer(1000)
    error_sig_buffer = misc.Buffer(4153)
    cpt_sig_buffer = misc.Buffer(4153)
    absorption_sig_buffer = misc.Buffer(300)
    marker1 = misc.Buffer(1)
    marker2 = misc.Buffer(1)
    marker3 = misc.Buffer(1)
    measurement_running = False;

    def __init__(self, serial):
        super().__init__()
        self.running = False
        self.serial = serial

    def run(self):
        self.running = True
        while self.running:
            try:
                line = self.serial.readline().decode('utf-8')
                if line:
                    if line[0] == 'D': # From printDebug() function
                        line = line[2:-2]
                    elif line[0] == 'I': # Time plot of the laser lock
                        self.current_buffer.append(time.time(),int(line[2:-2]))
                    elif line[0] == 'L': # Right lock modulation marker
                        self.marker1.append(time.time(),int(line[2:-2]))
                    elif line[0] == 'M': # Left lock modulation marker
                        self.marker2.append(time.time(),int(line[2:-2]))
                    elif line[0] == 'N': # Quartz frequency offset lock marker
                        self.marker3.append(time.time(),int(line[2:-2]))
                    elif line[0] == 'Q': # Time plot of the Quartz frequency lock
                        self.quartzword_buffer.append(time.time(),int(line[2:-2]))
                    elif line[0] == 'E': # Error signal
                        dacw_error_array = line[2:-2].split(' ')
                        self.error_sig_buffer.append(int(dacw_error_array[0]),
                                                     float(dacw_error_array[1]))
                    elif line[0] == 'C': # CPT signal
                        dacw_error_array = line[2:-2].split(' ')
                        self.cpt_sig_buffer.append(int(dacw_error_array[0]),
                                                     float(dacw_error_array[1]))
                    elif line[0] == 'A': # Absorption signal
                        dacw_absorption = line[2:-2].split(' ')
                        self.absorption_sig_buffer.append(
                                (int(dacw_absorption[0])),
                               (float(dacw_absorption[1]))
                            )
                    elif line[0] == 'x': # Process stopped signal
                        self.measurement_running=False
                    self.new_value.emit(line)
            except KeyboardInterrupt:
                pass
            except Exception as ex:
                print(ex)
                self.serial.flushInput()
                time.sleep(0.1)
        self.serial.disconnect()


# ============================================================================
class SerialConsole(pg.LayoutWidget):

    new_command = pg.QtCore.Signal(str)

    def __init__(self, window=None):
        super().__init__()
        self.console = QtGui.QTextEdit(window)
        self.console.setReadOnly(True)
        p = self.console.palette()
        p.setColor(QtGui.QPalette.Base, QtGui.QColor('black'))
        p.setColor(QtGui.QPalette.Text, QtGui.QColor('lightgray'))
        self.console.setPalette(p)
        self.console.document().setMaximumBlockCount(500)
        self.command_box = QtGui.QLineEdit(self)
        self.send_button = QtGui.QPushButton('Send')
        self.addWidget(self.command_box, row=0, col=0)
        self.addWidget(self.send_button, row=0, col=1)
        self.addWidget(self.console, row=1, col=0, colspan=2)
        self.send_button.clicked.connect(self.send_command)

    def append_text(self, data):
        self.console.append(data)

    def send_command(self):
        command = self.command_box.text()
        self.new_command.emit(command)
        self.command_box.setText("")
        logging.debug("send_command %r", command)


# ============================================================================
class ControlWidget(pg.LayoutWidget):

    def __init__(self, device, serial_reader, window=None):
        super().__init__(window)
        self.mac = device
        self.sreader = serial_reader
        self.window = window
        # Variables monitored by the plots to show or hide markers
        self.visible_max_abs_marker=True
        self.visible_laser_mod_marker=True
        self.visible_quartz_error_marker=True
        self.enable_time_plot_autoscale=True
        self.enable_absorption_autoscale=True
        self.enable_error_autoscale=True
        self.error_limits=[misc.word_to_frequency(self.mac.start_quartz),misc.word_to_frequency(self.mac.end_quartz)]
        #
       
        self.p = Parameter.create(name='params', type='group')
        self.startup = Parameter.create(name='Start Up', type='action',visible=False)
        self.p.addChild(self.startup)
        ## New controls:
        # Group 1: Laser and magnetic field control
        group_1=pTypes.GroupParameter(name='Laser and magnetic field control')
        children_list=[]
        sub_children_list=[]
        #Textbox: Laser current laser
        self.set_laser_frequency = pTypes.SimpleParameter.create(
            name='Laser current',
            type='float',
            value=misc.word_to_current(self.mac.laser_lock_initial_value),
            step = 0.001,
            min = 0,
            max = 1.7,
            decimals=6,
            suffix='mA')
        children_list.append(self.set_laser_frequency)
        #Textbox: Magnetic field current
        self.mag_current = Parameter.create(
            name='Magnetic field current     ', 
            type='float',
            value=self.mac.magcursour.current,
            step = 0.1, 
            suffix='mA')
        children_list.append(self.mag_current)
        # Sub-group 1: Laser scan
        #Adding sub-group
        group_1_1=pTypes.GroupParameter(name='Laser scan')
        #Textbox: Start current
        self.set_ramp_start = pTypes.SimpleParameter.create(
            name='Start current',
            type='float',
            value=misc.word_to_current(self.mac.start_current),
            step = 0.01,
            max = 1.7, #Check
            min = 0,
            decimals=6,
            suffix='mA')
        sub_children_list.append(self.set_ramp_start)
        #Textbox: End current
        self.set_ramp_end = pTypes.SimpleParameter.create(
            name='End current',
            type='float',
            value=misc.word_to_current(self.mac.end_current),
            step = 0.01,
            max = 1.7,
            min = 0,
            decimals=6,
            suffix='mA')
        sub_children_list.append(self.set_ramp_end)
        #Textbox: Number of samples. Will be fixed to 300, hidden control
        self.set_ramp_samples =  pTypes.SimpleParameter.create(
            name='Samples',
            type='int',
            value=self.mac.number_samples,
            step = 10,
            visible=False
            )
        sub_children_list.append(self.set_ramp_samples)
        #Button: Start ramp
        self.btn_start_ramp = Parameter.create(
            name='Start ramp',
            type='action')
        sub_children_list.append(self.btn_start_ramp)
        #Add controls to sub-group list
        group_1_1.addChildren(sub_children_list)
        #Add sub-group to main group list
        children_list.append(group_1_1)
        
        # self.set_laser_temperature = pTypes.SimpleParameter.create( #Not used, controlled outside.
        #     name='Temperature*',
        #     type='float',
        #     # value=self.mac.adf.total_freq_ramp,
        #     step = 0.1,
        #     suffix='ohm')
        # children_list.append(self.set_laser_temperature) #Not working
        # gp2 = pTypes.GroupParameter(name='Magnetic field current source')
       
        
        #Adding content of main group
        group_1.addChildren(children_list)
        #Add group to panel
        self.p.addChild(group_1)
        
        # Group 2: Laser lock
        group_2=pTypes.GroupParameter(name='Laser lock')
        children_list=[]
        #Textbox: Initial guess for lock
        self.set_laser_lock_initial_value = pTypes.SimpleParameter.create(
            name='Initial guess',
            type='float',
            value=misc.word_to_current(self.mac.laser_lock_initial_value),
            step = 0.001,
            max = 1.7,
            min = 0,
            decimals=6,
            suffix='mA')
        children_list.append(self.set_laser_lock_initial_value)
        #Textbox: Modulation width
        self.set_laser_mod_width = pTypes.SimpleParameter.create(
            name='Modulation width',
            type='float',
            value=misc.word_to_current(self.mac.laser_mod_width)/1000,
            step =(3.8147e-05)/1000,
            max = 1.7,
            min = (3.8147e-05)/1000,
            decimals=6,
            siPrefix=True,
            suffix='A')
        children_list.append(self.set_laser_mod_width)
        #Textbox: kp parameter
        self.set_laser_kp = pTypes.SimpleParameter.create(
            name='kp',
            type='float',
            value=self.mac.laser_kp,
            step = 0.001,
            min = 0.001,
            decimals=6)
        children_list.append(self.set_laser_kp)
        #Textbox: ki parameter (Hidden)
        self.set_laser_ki = pTypes.SimpleParameter.create(
            name='ki',
            type='float',
            value=0,
            step = 0.01,
            min = 0,
            decimals=6,
            visible=False) # Hidden
        children_list.append(self.set_laser_ki)
        #Textbox: kd parameter (Hidden)
        self.set_laser_kd = pTypes.SimpleParameter.create(
            name='kd',
            type='float',
            value=0,
            step = 0.01,
            min = 0,
            decimals=6,
            visible=False)# Hidden
        children_list.append(self.set_laser_kd)
        #Button: Start lock
        self.btn_start_laser_lock = Parameter.create(
            name='Lock laser',
            type='action')
        children_list.append(self.btn_start_laser_lock)
        group_2.addChildren(children_list)
        self.p.addChild(group_2)
        
        # Group 3: Quartz control
        group_3=pTypes.GroupParameter(name='Quartz control')
        children_list=[]

        # Adding Sub-group 1: Error signal plot
        group_3_1=pTypes.GroupParameter(name='Error signal plot')
        sub_children_list=[]
        #Textbox: Start frequency
        self.set_start_quartz = pTypes.SimpleParameter.create(
            name='Start frequency',
            type='float',
            value=misc.word_to_frequency(self.mac.start_quartz),
            step = 1,
            min = 0,
            decimals=6,
            suffix='Hz')
        sub_children_list.append(self.set_start_quartz)
        #Textbox: End frequency
        self.set_end_quartz = pTypes.SimpleParameter.create(
            name='End frequency',
            type='float',
            value=misc.word_to_frequency(self.mac.end_quartz),
            step = 1,
            min = 0,
            decimals=6,
            suffix='Hz')
        sub_children_list.append(self.set_end_quartz)
        #Button: Show error signal
        self.btn_start_quartz_lock = Parameter.create(
            name="Show Error",
            type='action')
        sub_children_list.append(self.btn_start_quartz_lock)
        #Add content of sub-group
        group_3_1.addChildren(sub_children_list)
        #Add sub-group to main group list
        children_list.append(group_3_1)

        # Adding Sub-group 2: CPT lock
        group_3_2=pTypes.GroupParameter(name='CPT lock')
        sub_children_list=[]
        #Textbox: Initial current guess 
        self.set_cpt_laser_lock_initial_value = pTypes.SimpleParameter.create(
            name='Initial current guess',
            type='float',
            value=misc.word_to_current(self.mac.laser_lock_initial_value),
            step = 0.001,
            max = 1.7,
            min = 0,
            decimals=6,
            suffix='mA')
        sub_children_list.append(self.set_cpt_laser_lock_initial_value)
        #Textbox: Initial frequency guess
        self.set_quartz_lock_initial_value = pTypes.SimpleParameter.create(
            name='Initial frequency guess    ',
            type='float',
            value=misc.word_to_frequency(self.mac.quartz_lock_initial_value),
            step = 1,
            min = 0,
            decimals=6,
            suffix='Hz')
        sub_children_list.append(self.set_quartz_lock_initial_value)
        #Textbox: Frequency offset
        self.set_quartz_frequency_offset = pTypes.SimpleParameter.create(
            name='Frequency offset    ',
            type='int',
            value=0,
            step = 1,
            decimals=6)
        sub_children_list.append(self.set_quartz_frequency_offset)
        #
        # self.set_quartz_mod_width = pTypes.SimpleParameter.create( #Hardcoded at 1kHz (PLL deviation)
        #     name='Modulation width*',
        #     type='float',
        #     value=1,
        #     step = 0.01,
        #     min = 1,
        #     decimals=6,
        #     suffix='Hz')
        # children_list.append(self.set_quartz_mod_width) #Not used, hidden
        #Textbox: kp parameter
        self.set_quartz_kp = pTypes.SimpleParameter.create(
            name='kp',
            type='float',
            value=self.mac.quartz_kp,
            step = 0.001,
            min = 0.001,
            decimals=6)
        sub_children_list.append(self.set_quartz_kp)
        #Textbox: ki parameter
        self.set_quartz_ki = pTypes.SimpleParameter.create(
            name='ki',
            type='float',
            value=self.mac.quartz_ki,
            step = 0.001,
            min = 0,
            decimals=6)
        sub_children_list.append(self.set_quartz_ki)
        #Textbox: kd parameter
        self.set_quartz_kd = pTypes.SimpleParameter.create(
            name='kd',
            type='float',
            value=self.mac.quartz_kd,
            step = 0.001,
            min = 0,
            decimals=6)
        sub_children_list.append(self.set_quartz_kd)
        #Button: Start lock
        self.btn_start_CPT_lock = Parameter.create(
            name='Lock CPT',
            type='action')
        sub_children_list.append(self.btn_start_CPT_lock)
        #Adding sub-group 2 content
        group_3_2.addChildren(sub_children_list) 
        #Adding sub-group to main group
        children_list.append(group_3_2)

        #Adding Sub-group 3: Monitoring
        group_3_3=pTypes.GroupParameter(name='Monitoring')
        sub_children_list=[]
        self.label_laser_lock_value = pTypes.SimpleParameter.create(
            name='Laser lock current',
            type='float',
            min = 0,
            decimals=6,
            readonly=True,
            suffix='mA')
        sub_children_list.append(self.label_laser_lock_value)
        self.label_quartz_lock_value = pTypes.SimpleParameter.create(
            name='Quartz lock frequency',
            type='float',
            min = 0,
            decimals=6,
            readonly=True,
            suffix='Hz')
        sub_children_list.append(self.label_quartz_lock_value)
        #Adding sub-group 3 content
        group_3_3.addChildren(sub_children_list)
        #Adding sub-group to main group
        children_list.append(group_3_3)

        #Adding content of main group
        group_3.addChildren(children_list)
        #Adding main group to panel
        self.p.addChild(group_3)

        # Group 4: Plotting tools
        group_4=pTypes.GroupParameter(name='Plot tools')
        # Sub-group 1: Time plots
        group_4_1=pTypes.GroupParameter(name='Time plots')
        children_list=[]
        self.set_time_plot_autoscale = Parameter.create(
            name='Auto scale',
            type='bool',
            value=True)
        children_list.append(self.set_time_plot_autoscale)
        group_4_1.addChildren(children_list)
        group_4.addChild(group_4_1)
        # Sub-group 2: Absorption plot
        group_4_2=pTypes.GroupParameter(name='Absorption plot')
        children_list=[]
        self.set_absorption_autoscale = Parameter.create(
            name='Auto scale',
            type='bool',
            value=True)
        children_list.append(self.set_absorption_autoscale)
        self.set_visible_max_abs_marker = Parameter.create(
            name='Show minimum',
            type='bool',
            value=True)
        children_list.append(self.set_visible_max_abs_marker)
        self.set_visible_laser_mod_marker = Parameter.create(
            name='Show modulation',
            type='bool',
            value=True)
        children_list.append(self.set_visible_laser_mod_marker)
        group_4_2.addChildren(children_list)
        group_4.addChild(group_4_2)
        # Sub-group 3: Error signal plot
        group_4_3=pTypes.GroupParameter(name='Error signal plot')
        children_list=[]
        self.set_error_autoscale = Parameter.create(
            name='Auto scale',
            type='bool',
            value=True)
        children_list.append(self.set_error_autoscale)
        self.set_visible_quartz_error_marker = Parameter.create(
            name='Show quartz lock',
            type='bool',
            value=True)
        children_list.append(self.set_visible_quartz_error_marker)
        group_4_3.addChildren(children_list)
        group_4.addChild(group_4_3)

        self.p.addChild(group_4)

        ## OLD CONTROLS, Not used:
        gp0 = pTypes.GroupParameter(name='ADF4158 (Frequency Synthesis)')
        self.set_all_regesiters = Parameter.create(
                name='Set registers',
                type='action')
        #self.set_frequency = MyFloatParameter.create(
        self.set_frequency = pTypes.SimpleParameter.create(
                name='Frequency',
                type='float',
                value=self.mac.adf.frequency,
                step = 1000,
                decimals = 12,
                suffix='Hz')
        self.ramp_on = Parameter.create(
                name='Ramp',
                type='bool',
                value=self.mac.adf.ramp_on)
        self.set_deviation = Parameter.create(
                name='Ramp deviation',
                type='float',
                value=self.mac.adf.total_freq_ramp,
                step = 1000,
                decimals = 6,
                suffix='Hz')
        self.clock2 = Parameter.create(
                name='Clock2 divider',
                type='int',
                value=self.mac.adf.clk2_divider_value,
                step = 1)
        self.ramp_steps = Parameter.create(
                name='Ramp steps', type='int',
                value=self.mac.adf.ramp_steps, step = 1)
        readonlygroup = pTypes.GroupParameter(name='Current values')
        #self.act_frequency = MyFloatParameter.create(
        self.act_frequency = pTypes.SimpleParameter.create(
                name='Frequency',
                #type='float2',
                value=self.mac.adf.frequency,
                readonly=True,
                suffix='Hz',
                enabled = False)
        self.act_deviation = Parameter.create(
                name='Ramp deviation',
                type='float',
                value=self.mac.adf.total_freq_ramp,
                readonly=True,
                suffix='Hz')
        #self.ramp_time = MyFloatParameter.create(
        self.ramp_time = pTypes.SimpleParameter.create(
                name='Total ramp time',
                #type='float2',
                value=self.mac.adf.total_time_ramp,
                readonly=True,
                suffix='s')
        readonlygroup.addChildren([self.act_frequency,
                                   self.act_deviation,
                                   self.ramp_time])
        gp0.addChildren([self.set_all_regesiters, self.set_frequency,
                         self.ramp_on, self.set_deviation,
                         self.clock2, self.ramp_steps,
                         readonlygroup])
        
        # self.p.addChild(gp0) # uncomment to ENABLE ADF CONTROL

        gp1 = pTypes.GroupParameter(name='Laser control old')
        self.laser_startup = Parameter.create(name='Startup', type='action')
        self.laser_current = Parameter.create(
            name='Current',
            type='float',
            value=self.mac.lascursour.current,
            step = 0.001, suffix='mA')
        self.tec_state = Parameter.create(
            name="TEC state",
            type="bool",
            value=self.mac.lastecctrl.tec_state)
        self.tec_volt_word = Parameter.create(
            name="TEC word",
            type="int",
            value=self.mac.lastecctrl.tec_volt_word,
            step = 1)
        self.tec_resistance = Parameter.create(
            name="TEC resistance",
            type="float",
            value=self.mac.lastecctrl.tec_resistance,
            step = 100,
            suffix="ohm")
        gp1.addChildren([self.laser_startup,
                         self.laser_current,
                         self.tec_state,
                         self.tec_volt_word,
                         self.tec_resistance])
        # self.p.addChild(gp1)

        # gp2 = pTypes.GroupParameter(name='Magnetic field current source')
        # self.mag_current = Parameter.create(name='Current', type='float',
        #                                     value=self.mac.magcursour.current,
        #                                     step = 0.1, suffix='mA')
        # gp2.addChildren([self.mag_current])
        # self.p.addChild(gp2)

        self.lockon = Parameter.create(name='Laser lock ON', type='action')
        # self.p.addChild(self.lockon)

        gp3 = pTypes.GroupParameter(name='Plotting tools')
        self.scope_state = Parameter.create(name='Acquisition', type='bool',
                                            value=False)
        self.clear_error_sig_action = Parameter.create(name='Clear',
                                                       type='action')
        gp3.addChildren([self.scope_state,self.clear_error_sig_action])
        # self.p.addChild(gp3)

        t = ParameterTree()
        t.setParameters(self.p, showTop=False)
        self.addWidget(t,row=0, col=0)

        self.p.sigTreeStateChanged.connect(self.change)
        # self.startup.sigActivated.connect(self.mac.startup)
        self.lockon.sigActivated.connect(self.mac.lock)
        self.set_all_regesiters.sigActivated.connect(
            self.mac.adf.send_all_reg)
        self.laser_startup.sigActivated.connect(self.mac.lascursour.startup)

        # Carlos RIVERA:
        # self.btn_start_ramp.sigActivated.connect(self.mac.start_ramp)
        self.btn_start_ramp.sigActivated.connect(self.action_start_ramp)
        self.btn_start_laser_lock.sigActivated.connect(self.action_start_laser_lock)
        self.btn_start_quartz_lock.sigActivated.connect(self.action_start_quartz_lock)
        self.btn_start_CPT_lock.sigActivated.connect(self.action_start_CPT_lock)
        
        def clear_error_sig_plot():
            self.sreader.error_sig_buffer.clear()

        self.clear_error_sig_action.sigActivated.connect(clear_error_sig_plot)

    def action_start_ramp(self):
        if("Start ramp" ==self.btn_start_ramp.name()):
            self.btn_start_ramp.setName("Stop ramp")
            self.mac.start_ramp()
        else:
            self.btn_start_ramp.setName("Start ramp")
            self.mac.stop_signal()
    
    def action_start_laser_lock(self):
        if("Lock laser" == self.btn_start_laser_lock.name()):
            self.btn_start_laser_lock.setName("Unlock laser")
            #Reset previous "initial guess" laser current
            self.mac.lascursour.reset()
            time.sleep(1)
            self.mac.lock_laser()
        else:
            self.btn_start_laser_lock.setName("Lock laser")
            self.mac.stop_signal()

    def action_start_quartz_lock(self):
        if("Show Error" == self.btn_start_quartz_lock.name()):
            self.btn_start_quartz_lock.setName("Stop Error")
            self.sreader.measurement_running=True
            self.sreader.error_sig_buffer.clear()
            self.sreader.cpt_sig_buffer.clear()
            self.mac.lock_quartz()
        else:
            self.btn_start_quartz_lock.setName("Show Error")
            self.sreader.measurement_running=False
            self.mac.stop_signal()
    
    def action_start_CPT_lock(self):
        if("Lock CPT" == self.btn_start_CPT_lock.name()):
            self.btn_start_CPT_lock.setName("Unlock CPT")
            self.mac.lock_cpt()
        else:
            self.btn_start_CPT_lock.setName("Lock CPT")
            self.mac.stop_signal()

    def change(self, param, changes):
        for param, change, data in changes:
            # Group 1: Laser and magnetic field control
            if param is self.set_laser_frequency:
                self.mac.lascursour.current= param.value()
                # Link three laser current controls:
                self.set_laser_frequency.setValue(param.value())
                self.set_laser_lock_initial_value.setValue(param.value())
                self.set_cpt_laser_lock_initial_value.setValue(param.value())
            # elif param is self.set_laser_temperature: # Not used
            #     pass # Not used
            elif param is self.mag_current:
                self.mac.magcursour.current = param.value()
            # Sub-group 1: Laser scan
            elif param is self.set_ramp_start:
                self.mac.start_current = misc.current_to_word(param.value())
            elif param is self.set_ramp_end:
                self.mac.end_current= misc.current_to_word(param.value())
            elif param is self.set_ramp_samples:
                self.mac.number_samples=param.value()
                self.sreader.absorption_sig_buffer = misc.Buffer(param.value())
            # Group 2: Laser lock
            elif param is self.set_laser_lock_initial_value:
                self.mac.laser_lock_initial_value = misc.current_to_word(param.value())
                # Link three laser current controls:
                self.mac.lascursour.current= param.value()
                self.set_laser_frequency.setValue(param.value())
                self.set_laser_lock_initial_value.setValue(param.value())
                self.set_cpt_laser_lock_initial_value.setValue(param.value())
            elif param is self.set_laser_mod_width:
                self.mac.laser_mod_width = misc.current_to_word(param.value()*1000/2)
            elif param is self.set_laser_kp:
                self.mac.laser_kp=param.value()
                self.mac.update_pid()
            elif param is self.set_laser_ki:
                pass
            elif param is self.set_laser_kd:
                pass
            # Group 3: Quartz control
            # Sub-group 1: Error signal plot
            elif param is self.set_start_quartz:
                self.mac.start_quartz = misc.frequency_to_word(param.value()) 
                self.mac.update_quartz_scan_parameters()
                self.error_limits[0]=param.value()
            elif param is self.set_end_quartz:
                self.mac.end_quartz =  misc.frequency_to_word(param.value()) 
                self.mac.update_quartz_scan_parameters()
                self.error_limits[1]=param.value()
            # Sub-group 2: CPT lock
            elif param is self.set_cpt_laser_lock_initial_value:
                # Link three laser current controls:
                self.mac.lascursour.current= param.value()
                self.set_laser_frequency.setValue(param.value())
                self.set_laser_lock_initial_value.setValue(param.value())
                self.set_cpt_laser_lock_initial_value.setValue(param.value())
            elif param is self.set_quartz_lock_initial_value:
                self.mac.quartz_lock_initial_value = misc.frequency_to_word(param.value()) 
                self.mac.update_quartz_parameters()
            elif param is self.set_quartz_frequency_offset:
                self.mac.quartz_frequency_offset = param.value()
                self.mac.update_quartz_parameters()
            # elif param is self.set_quartz_mod_width:
                # self.mac.laser_mod_width = param.value() #Check! Conversion pending
            elif param is self.set_quartz_kp:
                self.mac.quartz_kp=param.value()
                self.mac.update_pid()
            elif param is self.set_quartz_ki:
                self.mac.quartz_ki=param.value()
                self.mac.update_pid()
            elif param is self.set_quartz_kd:
                self.mac.quartz_kd=param.value()
                self.mac.update_pid()
            #  Sub-group 3: Monitoring (Read only, do not change)
            # Group 4: Plotting tools
            # Sub-group 1: Time plots
            elif param is self.set_time_plot_autoscale:
                self.enable_time_plot_autoscale=param.value()
            # Sub-group 2: Absorption plot
            elif param is self.set_absorption_autoscale:
                self.enable_absorption_autoscale=param.value()
            elif param is self.set_visible_max_abs_marker:
                self.visible_max_abs_marker=param.value()
            elif param is self.set_visible_laser_mod_marker:
                self.visible_laser_mod_marker=param.value()
            # Sub-group 3: Error signal plot
            elif param is self.set_error_autoscale:
                self.enable_error_autoscale=param.value()
            elif param is self.set_visible_quartz_error_marker:
                self.visible_quartz_error_marker=param.value()
            
        # old controls, not used:
        # for param, change, data in changes:
            elif param is self.set_frequency:
                self.mac.adf.frequency = param.value()
                self.act_frequency.setValue(self.mac.adf.frequency)
            elif param is self.ramp_on: self.mac.adf.ramp_on = param.value()
            elif param is self.set_deviation:
                self.mac.adf.total_freq_ramp = param.value()
                self.act_deviation.setValue(self.mac.adf.total_freq_ramp)
            elif param is self.clock2:
                self.mac.adf.clk2_divider_value = param.value()
                self.ramp_time.setValue(self.mac.adf.total_time_ramp)
            elif param is self.ramp_steps:
                self.mac.adf.ramp_steps = param.value()
                self.ramp_time.setValue(self.mac.adf.total_time_ramp)
                self.act_deviation.setValue(self.mac.adf.total_freq_ramp)
            elif param is self.laser_current:
                self.mac.lascursour.current = param.value()
            elif param is self.tec_state:
                self.mac.lastecctrl.tec_state = param.value()
            elif param is self.tec_volt_word:
                self.mac.lastecctrl.tec_volt_word = param.value()
            elif param is self.tec_resistance:
                self.mac.lastecctrl.tec_resistance = param.value()
            # elif param is self.mag_current:
            #     self.mac.magcursour.current = param.value()
            elif param is self.scope_state:
                if param.value():
                    self.window.live_graph_widget.start_timer()
                    self.rp_scope.start()
                else:
                    self.window.live_graph_widget.stop_timer()
                    self.rp_scope.stop()


# ============================================================================
class LiveGraphWidget(pg.GraphicsLayoutWidget):

    def __init__(self, serial_reader, control, window=None):
        super().__init__(window)
        self.control = control
        self.sreader = serial_reader
        self.p1 = self.addPlot()
        self.p1.showGrid(x=False, y=True, alpha=0.8)
        self.p1.setLabel(axis='left', text='Laser current (mA)')
        self.p1.setLabel(axis='bottom', text='Time (s)')
        self.curve1 = self.p1.plot(pen=pg.mkPen((0, 255, 155,255)))
        self.nextRow()
        self.p2 = self.addPlot()
        self.p2.showGrid(x=False, y=True, alpha=0.8)
        self.p2.setLabel(axis='left', text='Quartz frequency offset (Hz)')
        self.p2.setLabel(axis='bottom', text='Time (s)')
        self.curve2 = self.p2.plot(pen=pg.mkPen((0, 155, 255,255)))
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.update)
        self.start_timer()
        
    def update(self):
        if self.sreader:
            y_laser=self.sreader.current_buffer.get_data()
            y_quartz=self.sreader.quartzword_buffer.get_data()
            # Modify received data laser
            try:
                for [i,value] in enumerate(y_laser):
                    ## Convert machine units to physical units
                    y_laser[i]=misc.word_to_current(y_laser[i])
            except:
                pass
            # Modify received data quartz
            try:
                for [i,value] in enumerate(y_quartz):
                    ## Convert machine units to physical units
                    y_quartz[i]=misc.word_to_frequency(y_quartz[i])
            except:
                pass
            # Current graph
            self.curve1.setData(y=y_laser)

            # Quartz graph
            self.curve2.setData(y=y_quartz)
        
        #Enable or disable autoscale
        if self.control.enable_time_plot_autoscale:
            self.p1.enableAutoRange()
            self.p2.enableAutoRange()
        else:
            self.p1.disableAutoRange()
            self.p2.disableAutoRange()

    def start_timer(self):
        self.timer.start(100)

    def stop_timer(self):
        self.timer.stop()

# ============================================================================
class ErrorGraphWidget(pg.GraphicsLayoutWidget):

    def __init__(self, serial_reader, control, window=None):
        super().__init__(window)
        self.control = control
        self.sreader = serial_reader
        self.p1 = self.addPlot()
        self.p1.showGrid(x=True, y=True, alpha=0.8)
        self.p1.setLabel(axis='left', text='Error signal')
        self.p1.setLabel(axis='bottom', text='Quartz frequency offset (Hz)')
        self.curve1 = self.p1.plot(pen=pg.mkPen((0, 155, 255,255)))
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.update)
        self.start_timer()
        # Create marker for locking frequency
        # Marker 1:
        self.text =pg.TextItem(" ", anchor=(0, -1.0))
        self.p1.addItem(self.text)
        self.text.setPos(1,1)
        self.arrow = pg.ArrowItem(angle=90)
        self.p1.addItem(self.arrow)
        self.arrow.setPos(0,0)

    def update(self):
        if self.sreader:            
            y = self.sreader.error_sig_buffer.get_data()
            x = self.sreader.error_sig_buffer.get_times()
            m_x=misc.word_to_frequency(self.sreader.marker3.get_data())
            # Sorting arrays based on x axis to avoid lines going from left to right
            x_ind = x.argsort()
            x = x[x_ind[::-1]]
            y = y[x_ind[::-1]]
            # Modify received data
            try:
                for [i,value] in enumerate(x):
                    ## Convert machine units to physical units
                    x[i]=misc.word_to_frequency(x[i])
                    pass
            except:
                pass

            n = min(len(x), len(y))
            if n > 1:
                self.curve1.setData(x=x[0:n], y=y[0:n])
                # Clear buffer and change button text if measurement is over:
                if not self.sreader.measurement_running:
                    self.control.btn_start_quartz_lock.setName("Show Error")
            # Move locking frequency marker based on global variable (Value received at "l" function)
            if(len(m_x)>0):
                try:
                    # Change position and text of arrow 1
                    self.text.setPos(m_x[0], self.control.set_quartz_frequency_offset.value())
                    self.arrow.setPos(m_x[0], self.control.set_quartz_frequency_offset.value())
                    if m_x[0]>self.control.error_limits[0] and m_x[0]<self.control.error_limits[1]:
                        self.text.setText('[f={:0.3f} Hz]'.format(m_x[0]))
                        self.arrow.setStyle(brush=pg.mkBrush('b'))
                    else:
                        self.text.setText('Out of scope')
                        self.arrow.setStyle(brush=pg.mkBrush('r'))
                     # Show current locking frequency in label on control panel
                    self.control.label_quartz_lock_value.setValue(str(m_x[0]))
                except:
                    pass
        #Enable or disable autoscale
        if self.control.enable_error_autoscale:
            self.p1.enableAutoRange()
        else:
            self.p1.disableAutoRange()
        #Show or hide marker depending on controls
        checked_state=self.control.visible_quartz_error_marker
        self.text.setVisible(checked_state)
        self.arrow.setVisible(checked_state)

    def start_timer(self):
        self.timer.start(100)

    def stop_timer(self):
        self.timer.stop()

# ============================================================================
class CPTGraphWidget(pg.GraphicsLayoutWidget):
    
    def __init__(self, serial_reader, control, window=None):
        super().__init__(window)
        self.control = control
        self.sreader = serial_reader
        self.p1 = self.addPlot()
        self.p1.showGrid(x=True, y=True, alpha=0.8)
        self.p1.setLabel(axis='left', text='Photodiode voltage (V)')
        self.p1.setLabel(axis='bottom', text='Quartz frequency offset (Hz)')
        self.curve1 = self.p1.plot(pen=pg.mkPen((0, 155, 255,255)))
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.update)
        self.start_timer()
        # Create marker for locking frequency
        # Marker 1:
        self.curvePoint = pg.CurvePoint(self.curve1)
        self.p1.addItem(self.curvePoint)
        self.text = pg.TextItem(" ", anchor=(0, -1.0))
        self.text.setParentItem(self.curvePoint)
        self.arrow = pg.ArrowItem(angle=90)
        self.arrow.setParentItem(self.curvePoint)

    def update(self):
        if self.sreader:            
            y = self.sreader.cpt_sig_buffer.get_data()
            x = self.sreader.cpt_sig_buffer.get_times()
            m_x=misc.word_to_frequency(self.sreader.marker3.get_data())
            # Sorting arrays based on x axis to avoid lines going from left to right
            x_ind = x.argsort()
            x = x[x_ind[::-1]]
            y = y[x_ind[::-1]]
            # Modify received data
            try:
                for [i,value] in enumerate(x):
                    ## Convert machine units to physical units
                    x[i]=misc.word_to_frequency(x[i])
                    y[i]=misc.word_to_voltage(y[i])
                    pass
            except:
                pass

            n = min(len(x), len(y))
            if n > 1:
                self.curve1.setData(x=x[0:n], y=y[0:n])
                # Clear buffer and change button text if measurement is over:
                if not self.sreader.measurement_running:
                    self.control.btn_start_quartz_lock.setName("Show Error")
            # Move locking frequency marker based on global variable (Value received at "l" function)
            if(len(m_x)>0):
                try:
                    # Change position and text of arrow 2
                    index = np.where(x < m_x[0])[0][0]
                    self.curvePoint.setPos(float(index)/(x.size-1))
                    self.text.setText('[f={:0.3f} Hz, v={:0.3f} V]'.format(x[index], y[index]))
                    self.arrow.setStyle(brush=pg.mkBrush('b'))
                except:
                    #out of curve, change color to red
                    self.text.setText('Out of scope')
                    self.arrow.setStyle(brush=pg.mkBrush('r'))
                    pass
        #Enable or disable autoscale
        if self.control.enable_error_autoscale:
            self.p1.enableAutoRange()
        else:
            self.p1.disableAutoRange()
        #Show or hide marker depending on controls
        checked_state=self.control.visible_quartz_error_marker
        self.text.setVisible(checked_state)
        self.arrow.setVisible(checked_state)

    def start_timer(self):
        self.timer.start(100)

    def stop_timer(self):
        self.timer.stop()

# ============================================================================
class AbsorptionGraphWidget(pg.GraphicsLayoutWidget):

    def __init__(self, serial_reader, control, window=None):
        super().__init__(window)
        self.control = control
        self.sreader = serial_reader
        self.p1 = self.addPlot()
        self.p1.showGrid(x=True, y=True, alpha=0.8)
        self.p1.setLabel(axis='left', text='Photodiode voltage (V)')
        self.p1.setLabel(axis='bottom', text='Laser current (mA)')
        self.curve1 = self.p1.plot(pen=pg.mkPen((0, 255, 155,255)))
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.update)
        self.start_timer()
        # Creating marker to indicate maximum absorption
        self.text = pg.TextItem(html='<div style="text-align: center"><span style="color: #FFF;">Maximum absorption at</span><br><span style="color: #FF0; font-size: 12pt;">0</span></div>', anchor=(-0.1,1.3), border='w', fill=(0, 0, 255, 100))
        self.p1.addItem(self.text)
        self.text.setPos(1,1)
        self.arrow = pg.ArrowItem(angle=-45)
        self.p1.addItem(self.arrow)
        self.arrow.setPos(0,0)
        # Marker 2:
        self.curvePoint = pg.CurvePoint(self.curve1)
        self.p1.addItem(self.curvePoint)
        self.text2 = pg.TextItem(" ", anchor=(0, -0.5))
        self.text2.setParentItem(self.curvePoint)
        self.arrow2 = pg.ArrowItem(angle=45)
        self.arrow2.setParentItem(self.curvePoint)
        # Marker 3:
        self.curvePoint2 = pg.CurvePoint(self.curve1)
        self.p1.addItem(self.curvePoint2)
        self.text3 = pg.TextItem(" ", anchor=(1, -0.5))
        self.text3.setParentItem(self.curvePoint2)
        self.arrow3 = pg.ArrowItem(angle=135)
        self.arrow3.setParentItem(self.curvePoint2)

    def update(self):
        if self.sreader:
            y = self.sreader.absorption_sig_buffer.get_data()
            x = self.sreader.absorption_sig_buffer.get_times()
            m_x_1 = self.sreader.marker1.get_data()
            m_x_2 = self.sreader.marker2.get_data()
            n = min(len(x), len(y))
            if n > 1:
                # Sorting arrays based on x axis to avoid lines going from left to right
                x_ind = x.argsort()
                x = x[x_ind[::-1]]
                y = y[x_ind[::-1]]
                # Modify received data
                # Correction of overflow
                thresh = 20000 #10000
                overflow_index=[]
                overflow_ranges=[]
                try:
                    #Determine overflow occurrences (index)
                    for i in range(1,len(y)):
                        d=np.abs(y[i] - y[i-1])
                        if d > thresh: 
                            overflow_index.append(i)
                        pass
                    #Determine overflow ranges
                    try:
                        if (len(overflow_index) % 2) != 0:
                            overflow_index.append(len(y))
                        for i in range(0,len(overflow_index),2):
                            overflow_ranges.append([overflow_index[i],overflow_index[i+1]])
                    except print(0):
                        pass
                    #Modify y values between ranges
                    try:
                        for overflow_range in overflow_ranges:
                            for i in range(overflow_range[0],overflow_range[1]):
                                y[i]=y[i]-65535
                                pass
                    except print(0):
                        pass
                    
                except print(0):
                    pass
                ## Convert machine units to physical units
                for [i,value] in enumerate(y):
                    y[i]=misc.word_to_voltage(y[i])
                    x[i]=misc.word_to_current(x[i])
                    pass
                # print(overflow_index)
                # print(overflow_ranges)

                self.curve1.setData(x=x[0:n], y=y[0:n])
                # Updating marker position and text
                x_of_max=x[np.where(y == y.min())[0][0]]
                self.text.setPos(x_of_max, y.min())
                self.text.setHtml('<div style="text-align: right"><span style="color: #FFF;">Maximum absorption at:</span><br><span style="color: #FF0; font-size: 12pt;">{:.3f} mA, {:.3f} V</span></div>'.format(x_of_max,y.min()))

                ## Draw an arrowhead next to the text box
                self.arrow.setPos(x_of_max, y.min())
         
            # Move locking frequency marker based on global variable (Value received at "l" function)
            if(len(m_x_1)>0):
                try:
                    # Convert x position to current
                    m_x_1=misc.word_to_current(m_x_1)
                    # Change position and text of arrow 1
                    index = np.where(x < m_x_1[0])[0][0]
                    self.curvePoint.setPos(float(index)/(x.size-1))
                    self.text2.setText('[i={:0.3f} mA, v={:0.3f} V]'.format(x[index], y[index]))
                    self.arrow2.setStyle(brush=pg.mkBrush('b'))
                except:
                    #out of curve, change color to red
                    self.text2.setText('Out of scope')
                    self.arrow2.setStyle(brush=pg.mkBrush('r'))
                    pass
            if(len(m_x_2)>0):
                try:
                    # Convert x position to current
                    m_x_2=misc.word_to_current(m_x_2)
                    # Change position and text of arrow 2
                    index = np.where(x < m_x_2[0])[0][0]
                    self.curvePoint2.setPos(float(index)/(x.size-1))
                    self.text3.setText('[i={:0.3f} mA, v={:0.3f} V]'.format(x[index], y[index]))
                    self.arrow3.setStyle(brush=pg.mkBrush('b'))
                except:
                    #out of curve, change color to red
                    self.text3.setText('Out of scope')
                    self.arrow3.setStyle(brush=pg.mkBrush('r'))
                    pass
            if(len(m_x_1)>0 and len(m_x_2)>0):
                try:
                    # Show current locking frequency in label on control panel
                    self.control.label_laser_lock_value.setValue(str(m_x_1[0]+np.abs(m_x_1[0]-m_x_2[0])/2))
                except:
                    pass
        #Enable or disable autoscale
        if self.control.enable_absorption_autoscale:
            self.p1.enableAutoRange()
        else:
            self.p1.disableAutoRange()
        #Show or hide marker depending on controls
        #Maximum absorption marker
        checked_state=self.control.visible_max_abs_marker
        self.text.setVisible(checked_state)
        self.arrow.setVisible(checked_state)
        #Modulation markers
        checked_state=self.control.visible_laser_mod_marker
        self.text2.setVisible(checked_state)
        self.text3.setVisible(checked_state)
        self.arrow2.setVisible(checked_state)
        self.arrow3.setVisible(checked_state)
   
    def start_timer(self):
        self.timer.start(100)

    def stop_timer(self):
        self.timer.stop()

# ============================================================================
class MyWindow(QtGui.QMainWindow):

    def __init__(self, device, serial_reader):
        super().__init__()
        self.setWindowTitle("MAC Demonstrator v1.7")
        self.resize(1400,900)

        self.control_widget = ControlWidget(device, serial_reader, self)
        self.live_graph_widget = LiveGraphWidget(serial_reader, self.control_widget, self)
        self.serial_console = SerialConsole(self)
        self.error_sig_widget = ErrorGraphWidget(serial_reader, self.control_widget, self)
        self.cpt_sig_widget = CPTGraphWidget(serial_reader, self.control_widget, self)
        self.absorption_widget = AbsorptionGraphWidget(serial_reader, self.control_widget, self)

        area = DockArea()
        self.setCentralWidget(area)

        d1 = Dock("Controls", size=(385, 1))
        d2 = Dock("Quartz frequency offset  |        Laser current", size=(1000, 1))
        d3 = Dock("Console", size=(1000, 0.5))
        d4 = Dock("Error signal", size=(500, 1))
        d5 = Dock("Absorption signal", size=(500, 1))
        d6 = Dock("CPT signal", size=(500, 1))

        area.addDock(d3, 'top')
        area.addDock(d1, 'bottom')
        area.addDock(d2, 'right',d1)
        area.addDock(d4, 'bottom',d2)
        area.addDock(d5, 'left',d4) # in order to tab with "Error signal" select option: above or below
        area.addDock(d6, 'above', d4) # above = CPT over Error, below = the opposite 

        d1.addWidget(self.control_widget)
        d2.addWidget(self.live_graph_widget)
        d3.addWidget(self.serial_console)
        d4.addWidget(self.error_sig_widget)
        d5.addWidget(self.absorption_widget)
        d6.addWidget(self.cpt_sig_widget)

        serial_reader.new_value.connect(self.serial_console.append_text)
        self.serial_console.new_command.connect(device.send)
      
    def keyPressEvent(self, event):
        if event.key() == QtCore.Qt.Key_Enter \
           or event.key() == QtCore.Qt.Key_Return:
            self.serial_console.send_command()


# ============================================================================
def ask_for_port():
    """Show a list of ports and ask the user for a choice. To make selection
    easier on systems with long device names, also allow the input of an index.
    """
    sys.stderr.write('\n--- Available ports:\n')
    ports = []
    for n, (port, desc, hwid) in enumerate(sorted(comports()), 1):
        sys.stderr.write('--- {:2}: {:20} {!r}\n'.format(n, port, desc))
        ports.append(port)
    while True:
        sys.stderr.write('--- Enter port index or full name: ')
        port = input('')
        try:
            index = int(port) - 1
            if not 0 <= index < len(ports):
                sys.stderr.write('--- Invalid index!\n')
                continue
        except ValueError:
            pass
        else:
            port = ports[index]
        return port


# =============================================================================
def configure_logging(level):
    """Configure logging:
    - (debug) messages logged to console
    - messages logged in a file located in the home directory
    """
    log_console_format = "%(asctime)s [%(threadName)-12.12s]" + \
        "[%(levelname)-6.6s] %(filename)s %(funcName)s (%(lineno)d): " + \
        "%(message)s"
    console_formatter = logging.Formatter(log_console_format)

    root_logger = logging.getLogger()
    root_logger.setLevel(level)

    console_handler = logging.StreamHandler()
    console_handler.setLevel(level)
    console_handler.setFormatter(console_formatter)
    root_logger.addHandler(console_handler)


# =============================================================================
def parse_cmd_line():
    """Parse command line.
    :returns: argparse namespace (object)
    """
    parser = argparse.ArgumentParser(description='DdsController')
    '''parser.add_argument('-ini', '--ini_file',
                        help="Specifiy ini file to use.")
    if path.isfile(ini_file) is True:
        check_ini_file(ini_file)  # see ddsctrl
    else:
        check_ini_file()'''
    parser.add_argument('-p', '--port',
                        help="Serial port of MAC device.")
    parser.add_argument('-d', '--debug', action='store_true',
                        help="Enable debug log.")
    args = parser.parse_args()
    return args


# ============================================================================
def main(port):
    mac = md.Mac()
    mac.connect(port)
    serial_reader = SerialReader(mac.ser)
    app = QtGui.QApplication(sys.argv)
    win = MyWindow(mac, serial_reader)
    serial_reader.start()
    win.show()
    sys.exit(app.exec_())


# ============================================================================
signal.signal(signal.SIGINT, signal.SIG_DFL)  # Ctrl-c closes the application

args = parse_cmd_line()
if args.debug is True:
    configure_logging(logging.DEBUG)
else:
    configure_logging(logging.INFO)
if args.port:
    port = args.port
else:
    port = ask_for_port()
    # port = "COM7" # For testing and rapid debugging

main(port)
