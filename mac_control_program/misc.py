# -*- coding: utf-8 -*-

import sys
import time
import pickle
from collections import deque
from collections import OrderedDict
import numpy as np
import yaml


class Buffer():

    def __init__(self, size):
        self.size = size
        self.deque = deque([], size)
        self.time_array = np.zeros(size)
        self.data_array = np.zeros(size)
        self.ptr = -1

    def append(self, time_, value):
        self.ptr = (self.ptr + 1) % self.size
        self.deque.append(self.ptr)
        self.time_array[self.ptr] = time_
        self.data_array[self.ptr] = value

    def get_times(self):
        return self.time_array[self.deque]

    def get_data(self):
        return self.data_array[self.deque]

    def get_last(self):
        return self.get_data()[-1]

    def get_average(self):
        return np.mean(self.get_data())

    def get_std(self):
        return np.std(self.get_data())

    def clear(self):
        self.deque.clear()
        self.ptr = -1


def printProgress(idx, list_, item=None, message="", unit=""):
    progress = idx/float(len(list_))
    sys.stdout.write("\r%s: %.0f %% (%s%s) %s" % \
                     (message,
                      100*progress,
                      str(item),
                      unit,
                      '#'*int(progress*16)+'-'*int(16-progress*16)))
    sys.stdout.flush()


def printDone():
    sys.stdout.write("[Done]\n")


def makeRandomColor():
    r = np.random.rand()  # Make random color
    #r = lambda: np.random.rand()  # Make random color
    return (r(),r(),r())


def makeHeader(params):
    return "Notes:" + ','.join(['='.join([str(element) for element in tup]) for tup in params.items()])


def makeYamlHeader(params):
    return "YAML:\n" + yaml.dump(params)


def readHeader(filename):
    parameters = []
    with open(filename, 'r') as f:
        first_line = f.readline()
        if first_line.find("# Notes")>-1:
            parameters = first_line.replace('\n','').split(":")[1].split(",")
            
            #parameters = dict([(param.split('=')[0], param.split('=')[1]) for param in parameters])
            parameters = {param.split('=')[0]:param.split('=')[1] for param in parameters}
            
            return parameters
        if first_line.find("# YAML:") > -1:
            lines = []
            line = f.readline()
            while line[0]=="#":
                lines.append(line[2:])
                line = f.readline()
            return yaml.load(''.join(lines))
        print("No header found in this file")
    return parameters


def saveYaml(params, filename):
    with open(filename,'w') as f:
        yaml.dump(params,f,default_flow_style=False)


def savePickle(params,filename):
    with open(filename, 'wb') as f:
        pickle.dump(params, f, -1)


def loadYaml(filename="params.yaml"):
    with open(filename,'r') as f:
        try:
            return yaml.load(f, Loader=yaml.CLoader)
        except Exception as ex:
            print("Unknown error while loading file: %r", ex)


def loadPickle(filename):
    with open(filename, 'rb') as f:
        return pickle.load(f)


def getAllParams(equipments):
    params = OrderedDict()
    params.update(loadYaml("params.yaml"))
    params.update({"date": time.strftime("%d-%m-%y")})
    params.update({"time": time.strftime("%H%M")})
    for equipment in equipments:
        params.update(equipment.getParams())
    return params


def saveThisConfig(equipmentList,comment=None):
    params = getAllParams(equipmentList)
    if comment:
        params.update({"comment":comment})
    filename = "%s %s %s(Config).yaml" % (params["cell"],
                                          params["date"],
                                          params["time"])
    saveYaml(params, filename=filename)
