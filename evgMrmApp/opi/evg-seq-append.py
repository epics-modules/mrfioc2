from org.csstudio.opibuilder.scriptUtil import PVUtil

delay = display.getWidget("InsertDelay").getPropertyValue("text")
code  = display.getWidget("InsertCode").getPropertyValue("text")

table = display.getWidget("SeqSet").getTable()

row = table.getRowCount()

table.insertRow(row)

table.setCellText(row, 0, delay)
table.setCellText(row, 1, code)
