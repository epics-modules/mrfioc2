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
        labels = ["Event Code", "Time Stamp"]
        self.ui.tableWidget.setHorizontalHeaderLabels(labels)

        self.connect(self.ui.pb_setSoftSeq, SIGNAL("clicked()"), self.setSoftSeq)
        self.connect(self.ui.rb_tsConvert, SIGNAL("clicked()"), self.setTsResConvert)
        self.connect(self.ui.rb_tsRaw, SIGNAL("clicked()"), self.setTsResRaw)
	
        self.arg1 = argv[1]
        self.arg2 = argv[2]
        self.tsResConvert = 0
        self.tsResRaw = 0
        print argv	
	
        heading = self.arg1 + " - Soft " + self.arg2
        self.ui.heading_label.setText(heading)
	
        pv = self.arg1 + ":" + self.arg2 + ":timestamp.INP"
        camonitor(pv, self.cb_timestampInp)

        pv = self.arg1 + ":EvtClkSpeed"
        camonitor(pv, self.cb_evtClkSpeed)

        pv = self.arg1 + ":" + self.arg2 + ":timestampInpMode"
        if(caget(pv)):
            self.ui.rb_tsRaw.setChecked(1);
        else:
            self.ui.rb_tsConvert.setChecked(1);

        self.arrayReadback()
	

    def arrayReadback(self):
        pvEC = self.arg1 + ":" + self.arg2 + ":eventCode"
        valueEC = caget(pvEC)

        pvTS = self.arg1 + ":" + self.arg2 + ":timestamp"
        valueTS = caget(pvTS)

        if valueEC.ok == True & valueTS.ok == True:
            for x in range(self.ui.tableWidget.rowCount()):
                item = QTableWidgetItem(QString.number(valueEC[x]))
                self.ui.tableWidget.setItem(x, 0, item)
                item = QTableWidgetItem(QString.number(valueTS[x]))
                self.ui.tableWidget.setItem(x, 1, item)


    def setTsResConvert(self):
            self.ui.label_tsRes.setText( '%e Seconds'%(self.tsResConvert) )


    def cb_timestampInp(self, value):
        value = value[value.find('@')+1:]
        self.tsResConvert = 1.0/int(value)
        if self.ui.rb_tsConvert.isChecked():
            self.setTsResConvert()


    def setTsResRaw(self):
            self.ui.label_tsRes.setText( '%e Seconds'%(self.tsResRaw) )


    def cb_evtClkSpeed(self, value):
        self.tsResRaw = 1.0/(value*1000000)
        if self.ui.rb_tsRaw.isChecked():
            self.setTsResRaw()

  		
    def setSoftSeq(self):
        self.setTimestampInpMode()       
        self.setEvtCode()
        self.setTimeStamp()


    def setTimestampInpMode(self):
        pv = self.arg1 + ":" + self.arg2 + ":timestampInpMode"
        if self.ui.rb_tsConvert.isChecked():
            caput(pv, 0)
        else:
            caput(pv, 1)
	

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
	  
        print args
        pv = self.arg1 + ":" + self.arg2 + ":eventCode"
        caput(pv, args)

      	
    def setTimeStamp(self):
        args = []
        for x in range(self.ui.tableWidget.rowCount()):
            item = self.ui.tableWidget.item(x,1)
            if item == None:
                break
            else:
                (val, OK) = item.text().toDouble()
		
            if val == 0:
                break
            args.append(val)
	  
        print args
        pv = self.arg1 + ":" + self.arg2 + ":timestamp" 
        caput(pv, args)
	
if __name__ == '__main__':
    app = cothread.iqt(argv = sys.argv)
    softSeq = evgSoftSeq(sys.argv)
    softSeq.show()
    cothread.WaitForQuit()
  
