from org.csstudio.opibuilder.scriptUtil import PVUtil

widget = display.getWidget("SeqSet")

table = widget.getTable()

while table.getRowCount()>0:
	table.deleteRow(0)
