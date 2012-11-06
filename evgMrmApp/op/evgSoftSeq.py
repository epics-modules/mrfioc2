#! /usr/bin/env python

from PyQt4 import QtCore as core
from PyQt4 import QtGui as gui
import sys
from PyQt4.QtCore import SIGNAL

import cothread
from cothread import catools, cadef
from cothread.catools import camonitor, caput

from ui_evgsoftseq import Ui_EvgSoftSeq

class evgSoftSeq(gui.QMainWindow):

    enableAll = core.pyqtSignal(bool)

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

        self.updated = False
        self.enableAll.connect(self.ui.tableWidget.setEnabled)
        self.enableAll.connect(self.ui.pb_setSequence.setEnabled)
        
        self.enableAll.emit(False)

        self.codes, self.times = [], []

        pv = self.prefix + "TsResolution-RB"
        camonitor(pv, self.sequenceRB, notify_disconnect=True)

        pv = self.prefix + "TsInpMode-RB"
        camonitor(pv, self.sequenceRB, notify_disconnect=True)

        pv = self.prefix + "EvtCode-RB"
        camonitor(pv, self.newCodes, notify_disconnect=True)

        pv = self.prefix + "Timestamp-RB"
        camonitor(pv, self.newTimes, notify_disconnect=True)

    def msg(self, msg, delay=0):
        self.ui.statusBar.showMessage(msg,delay)

    def newCodes(self, codes):
        if codes.ok:
            self.codes = list(codes)
        self.sequenceRB(codes)

    def newTimes(self, times):
        if times.ok:
            self.times = list(times)
        self.sequenceRB(times)

    def sequenceRB(self, value):
        if not value.ok:
            self.enableAll.emit(False)
            self.msg("Connection lost")
            return
        elif len(self.codes)!=len(self.times) or len(self.codes)==0:
            self.enableAll.emit(False)
            return
        elif self.updated:
            return

        valueEC, valueTS = self.codes, self.times

        for x in range(self.ui.tableWidget.rowCount()):
            try:
                EC, TS = valueEC[x], valueTS[x]
            except IndexError:
                break
            if EC==127 or (EC==0 and TS==0):
                break
            item = gui.QTableWidgetItem(core.QString.number(EC))
            self.ui.tableWidget.setItem(x, 0, item)
            item = gui.QTableWidgetItem(core.QString.number(TS, "G", 14))
            self.ui.tableWidget.setItem(x, 1, item)

        self.enableAll.emit(True)
        self.msg("Updated",1000)

    def setSequence(self):        
        T, C = [], []
        for x in range(self.ui.tableWidget.rowCount()): 
            nextC = self.ui.tableWidget.item(x,0)
            nextT = self.ui.tableWidget.item(x,1)

            if nextC is None and nextT is None:
                break # Done
            elif nextC is None or nextT is None:
                self.msg("Incomplete sequence.")
                return

            nextC, nextT = str(nextC.text()), str(nextT.text())

            if not nextC and not nextT:
                break # Done
            elif not nextC or not nextT:
                self.msg("Incomplete sequence.")
                return

            try:
                nextC = int(nextC,0)
                if nextC <= 0 or nextC > 255:
                    self.msg("Code %d must be in range [0,255]"%nextC)
                    return
            except ValueError:
                self.msg("Code '%s' must be an integer"%nextC)
                return
            try:
                nextT = float(nextT)
                if nextT <= 0:
                    self.msg("Time %d must be >0"%nextT)
                    return
            except ValueError:
                self.msg("Time '%s' must be a positive number"%nextT)
                return

            T.append(nextT)
            C.append(nextC)

        self.codes, self.times = [], []

        self.msg('Sending...', 4000)
        cothread.Spawn(self._setpvs, T, C)

    def _setpvs(self, T, C):

        pvs =  [self.prefix + "EvtCode-SP", self.prefix + "Timestamp-SP"]
        try:
            self.enableAll.emit(False)
            caput(pvs, [C,T], timeout=2.0, wait=True)
            self.msg("Sent", 2000)
        except catools.ca_nothing,E:
            if E.errorcode==cadef.ECA_TIMEOUT:
                M='Timeout'
            else:
                M=cadef.ca_message(E.errorcode)
            self.msg("Sent failed: "+M, 4000)
            return
        finally:
            self.enableAll.emit(True)

if __name__ == '__main__':
    app = cothread.iqt(argv = sys.argv)
    softSeq = evgSoftSeq(sys.argv)
    softSeq.show()
    cothread.WaitForQuit()
  
