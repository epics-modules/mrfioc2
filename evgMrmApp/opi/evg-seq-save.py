from org.csstudio.opibuilder.scriptUtil import PVUtil, ConsoleUtil
from jarray import array

Timestamp_SP = pvs[0]
EvtCode_SP = pvs[1]

T, E = [], []
for row in widget.getTable().getContent():
	T.append(float(row[0]))
	E.append(float(row[1]))

Timestamp_SP.setValue(array(T, 'd'))
EvtCode_SP.setValue(array(E, 'd'))
