# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'evgsoftseq.ui'
#
# Created: Tue Dec 14 11:55:20 2010
#      by: PyQt4 UI code generator 4.4.2
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_EvgSoftSeq(object):
    def setupUi(self, EvgSoftSeq):
        EvgSoftSeq.setObjectName("EvgSoftSeq")
        EvgSoftSeq.resize(304,350)
        self.centralWidget = QtGui.QWidget(EvgSoftSeq)
        self.centralWidget.setObjectName("centralWidget")
        self.heading_label = QtGui.QLabel(self.centralWidget)
        self.heading_label.setGeometry(QtCore.QRect(9,9,181,17))
        font = QtGui.QFont()
        font.setPointSize(14)
        font.setWeight(75)
        font.setBold(True)
        self.heading_label.setFont(font)
        self.heading_label.setObjectName("heading_label")
        self.tableWidget = QtGui.QTableWidget(self.centralWidget)
        self.tableWidget.setGeometry(QtCore.QRect(9,32,256,192))
        self.tableWidget.setObjectName("tableWidget")
        self.setSoftSeq_EC_TS_PB = QtGui.QPushButton(self.centralWidget)
        self.setSoftSeq_EC_TS_PB.setGeometry(QtCore.QRect(9,230,85,27))
        self.setSoftSeq_EC_TS_PB.setObjectName("setSoftSeq_EC_TS_PB")
        EvgSoftSeq.setCentralWidget(self.centralWidget)
        self.menuBar = QtGui.QMenuBar(EvgSoftSeq)
        self.menuBar.setGeometry(QtCore.QRect(0,0,304,21))
        self.menuBar.setObjectName("menuBar")
        EvgSoftSeq.setMenuBar(self.menuBar)
        self.mainToolBar = QtGui.QToolBar(EvgSoftSeq)
        self.mainToolBar.setObjectName("mainToolBar")
        EvgSoftSeq.addToolBar(QtCore.Qt.TopToolBarArea,self.mainToolBar)
        self.statusBar = QtGui.QStatusBar(EvgSoftSeq)
        self.statusBar.setObjectName("statusBar")
        EvgSoftSeq.setStatusBar(self.statusBar)
        self.toolBar = QtGui.QToolBar(EvgSoftSeq)
        self.toolBar.setObjectName("toolBar")
        EvgSoftSeq.addToolBar(QtCore.Qt.TopToolBarArea,self.toolBar)

        self.retranslateUi(EvgSoftSeq)
        QtCore.QMetaObject.connectSlotsByName(EvgSoftSeq)

    def retranslateUi(self, EvgSoftSeq):
        EvgSoftSeq.setWindowTitle(QtGui.QApplication.translate("EvgSoftSeq", "EvgSoftSeq", None, QtGui.QApplication.UnicodeUTF8))
        self.heading_label.setText(QtGui.QApplication.translate("EvgSoftSeq", "TextLabel", None, QtGui.QApplication.UnicodeUTF8))
        self.tableWidget.setRowCount(2047)
        self.tableWidget.setColumnCount(2)
        self.tableWidget.clear()
        self.tableWidget.setColumnCount(2)
        self.tableWidget.setRowCount(2047)
        self.setSoftSeq_EC_TS_PB.setText(QtGui.QApplication.translate("EvgSoftSeq", "Set", None, QtGui.QApplication.UnicodeUTF8))
        self.toolBar.setWindowTitle(QtGui.QApplication.translate("EvgSoftSeq", "toolBar", None, QtGui.QApplication.UnicodeUTF8))

