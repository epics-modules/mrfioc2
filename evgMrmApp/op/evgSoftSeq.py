#! /usr/bin/env python

from PyQt4 import QtCore as core
from PyQt4 import QtGui as gui
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
    self.connect(self.ui.setSoftSeq_EC_TS_PB, SIGNAL("clicked()"), self.set)

    self.arg1 = argv[1]
    self.arg2 = argv[2]
    print argv	
	
    heading = self.arg1 + " - Soft " + self.arg2
    self.ui.heading_label.setText(heading)

  def set(self):
    self.setEvtCode()
    self.setTimeStamp()
	
  def setEvtCode(self):
    args = []
    for x in range(self.ui.tableWidget.rowCount()):    
      item = self.ui.tableWidget.item(x,0)
      if item == None:
        break
      else:
        (val, OK) = item.text().toInt()
		
      if val == 0:
        tsItem = self.ui.tableWidget.item(x,1)
        if item == None:
          break
        else:
          (tsVal, tsOK) = item.text().toInt()
          if tsVal != 255:
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
        (val, OK) = item.text().toFloat()
		
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
  