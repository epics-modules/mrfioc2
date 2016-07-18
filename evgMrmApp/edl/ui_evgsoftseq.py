# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'evgsoftseq.ui'
#
# Created: Wed Sep 28 15:54:06 2011
#      by: PyQt4 UI code generator 4.7.3
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_EvgSoftSeq(object):
    def setupUi(self, EvgSoftSeq):
        EvgSoftSeq.setObjectName("EvgSoftSeq")
        EvgSoftSeq.resize(295, 336)
        self.centralWidget = QtGui.QWidget(EvgSoftSeq)
        self.centralWidget.setObjectName("centralWidget")
        self.verticalLayout_2 = QtGui.QVBoxLayout(self.centralWidget)
        self.verticalLayout_2.setObjectName("verticalLayout_2")
        self.verticalLayout = QtGui.QVBoxLayout()
        self.verticalLayout.setObjectName("verticalLayout")
        self.label_Heading = QtGui.QLabel(self.centralWidget)
        font = QtGui.QFont()
        font.setPointSize(14)
        font.setWeight(75)
        font.setBold(True)
        self.label_Heading.setFont(font)
        self.label_Heading.setObjectName("label_Heading")
        self.verticalLayout.addWidget(self.label_Heading)
        self.tableWidget = QtGui.QTableWidget(self.centralWidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.tableWidget.sizePolicy().hasHeightForWidth())
        self.tableWidget.setSizePolicy(sizePolicy)
        self.tableWidget.setRowCount(2047)
        self.tableWidget.setColumnCount(2)
        self.tableWidget.setObjectName("tableWidget")
        self.tableWidget.setColumnCount(2)
        self.tableWidget.setRowCount(2047)
        self.tableWidget.horizontalHeader().setCascadingSectionResizes(True)
        self.tableWidget.verticalHeader().setCascadingSectionResizes(True)
        self.verticalLayout.addWidget(self.tableWidget)
        self.horizontalLayout = QtGui.QHBoxLayout()
        self.horizontalLayout.setObjectName("horizontalLayout")
        self.pb_setSequence = QtGui.QPushButton(self.centralWidget)
        self.pb_setSequence.setObjectName("pb_setSequence")
        self.horizontalLayout.addWidget(self.pb_setSequence)
        spacerItem = QtGui.QSpacerItem(148, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)
        self.verticalLayout.addLayout(self.horizontalLayout)
        self.verticalLayout_2.addLayout(self.verticalLayout)
        EvgSoftSeq.setCentralWidget(self.centralWidget)
        self.menuBar = QtGui.QMenuBar(EvgSoftSeq)
        self.menuBar.setGeometry(QtCore.QRect(0, 0, 295, 21))
        self.menuBar.setObjectName("menuBar")
        EvgSoftSeq.setMenuBar(self.menuBar)
        self.mainToolBar = QtGui.QToolBar(EvgSoftSeq)
        self.mainToolBar.setObjectName("mainToolBar")
        EvgSoftSeq.addToolBar(QtCore.Qt.ToolBarArea(QtCore.Qt.TopToolBarArea), self.mainToolBar)
        self.statusBar = QtGui.QStatusBar(EvgSoftSeq)
        self.statusBar.setObjectName("statusBar")
        EvgSoftSeq.setStatusBar(self.statusBar)
        self.toolBar = QtGui.QToolBar(EvgSoftSeq)
        self.toolBar.setObjectName("toolBar")
        EvgSoftSeq.addToolBar(QtCore.Qt.ToolBarArea(QtCore.Qt.TopToolBarArea), self.toolBar)

        self.retranslateUi(EvgSoftSeq)
        QtCore.QMetaObject.connectSlotsByName(EvgSoftSeq)

    def retranslateUi(self, EvgSoftSeq):
        EvgSoftSeq.setWindowTitle(QtGui.QApplication.translate("EvgSoftSeq", "EvgSoftSeq", None, QtGui.QApplication.UnicodeUTF8))
        self.label_Heading.setText(QtGui.QApplication.translate("EvgSoftSeq", "TextLabel", None, QtGui.QApplication.UnicodeUTF8))
        self.pb_setSequence.setText(QtGui.QApplication.translate("EvgSoftSeq", "Set", None, QtGui.QApplication.UnicodeUTF8))
        self.toolBar.setWindowTitle(QtGui.QApplication.translate("EvgSoftSeq", "toolBar", None, QtGui.QApplication.UnicodeUTF8))

