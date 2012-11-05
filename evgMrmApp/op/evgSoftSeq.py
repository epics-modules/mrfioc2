#! /usr/bin/env python

from PyQt4 import QtCore as core
from PyQt4.QtCore import *
from PyQt4 import QtGui as gui
from PyQt4.QtGui import *
import sys
import os
from subprocess import *
from PyQt4.QtCore import SIGNAL

import cothread
from cothread.catools import *

from ui_evgsoftseq import Ui_EvgSoftSeq

class evgSoftSeq(gui.QMainWindow):	
    def __init__(self, argv, parent=None):
        gui.QMainWindow.__init__(self, parent)
    
        self.ui = Ui_EvgSoftSeq()
        self.ui.setupUi(self)
        self.srcText=None

        labels = ["Event Code", "Timestamp"]
        self.ui.tableWidget.setHorizontalHeaderLabels(labels)

        if len(argv)==4:
            self.prefix='%s{%s-%s}' % tuple(argv[1:])
        elif len(argv)==2:
            self.prefix=argv[1]
        else:
            print "Invalid number of arguments"
            exit(0)

        self.ui.label_Heading.setText(self.prefix)

        self.connect(self.ui.pb_setSequence, SIGNAL("clicked()"), self.setSequence)

        pv = self.prefix + "TsResolution-RB"
        camonitor(pv, self.sequenceRB)

        pv = self.prefix + "TsInpMode-RB"
        camonitor(pv, self.sequenceRB)

        self.sequenceRB(0)
	

    def sequenceRB(self, value):
        pvEC = self.prefix + "EvtCode-RB"
        pvTS = self.prefix + "Timestamp-RB"
        valueEC, valueTS = caget([pvEC, pvTS])

        for x in range(self.ui.tableWidget.rowCount()):
            EC, TS = valueEC[x], valueTS[x]
            if EC==127 or (EC==0 and TS==0):
                break
            item = QTableWidgetItem(QString.number(EC))
            self.ui.tableWidget.setItem(x, 0, item)
            item = QTableWidgetItem(QString.number(TS, "G", 14))
            self.ui.tableWidget.setItem(x, 1, item)
	
    def setSequence(self):
        self.setEvtCode()
        self.setTimestamp()

    def setEvtCode(self):
        args = []
        for x in range(self.ui.tableWidget.rowCount()): 
            item = self.ui.tableWidget.item(x,0)
            if item == None:
                break
            else:
                (val, OK) = item.text().toInt()
 
            if val <= 0 or val > 255:
                break
            args.append(val)
	  
        pv =  self.prefix + "EvtCode-SP"
        caput(pv, args)

      	
    def setTimestamp(self):
        args = []
        for x in range(self.ui.tableWidget.rowCount()):
            item = self.ui.tableWidget.item(x,1)
            if item == None:
                break
            else:
                (val, OK) = item.text().toDouble()
		
            if val == 0 and x > 0:
                break
            args.append(val)
	  
        pv =  self.prefix + "Timestamp-SP"
        caput(pv, args)

if __name__ == '__main__':
    app = cothread.iqt(argv = sys.argv)
    softSeq = evgSoftSeq(sys.argv)
    softSeq.show()
    cothread.WaitForQuit()
  
