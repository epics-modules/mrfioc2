from org.csstudio.opibuilder.scriptUtil import PVUtil, ConsoleUtil

table = widget.getTable()

times = PVUtil.getDoubleArray(pvs[0])
codes = PVUtil.getDoubleArray(pvs[1])

N = min(len(times), len(codes))

while table.getRowCount()<N:
	table.insertRow(table.getRowCount())

while table.getRowCount()>N:
	table.deleteRow(table.getRowCount()-1)

times, codes = times[:N], codes[:N]

for row,(T,C) in enumerate(zip(times, codes)):
	table.setCellText(row,0,str(T))
	table.setCellText(row,1,"%d"%C)
