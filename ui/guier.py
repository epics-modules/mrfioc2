# -*- coding: utf-8 -*-
"""
Created on Fri Feb 27 15:29:23 2015

@author: sskube
"""

try:
  from lxml import etree
  print("running with lxml.etree")
except ImportError:
  try:
    # Python 2.5
    import xml.etree.cElementTree as etree
    print("running with cElementTree on Python 2.5+")
  except ImportError:
    try:
      # Python 2.5
      import xml.etree.ElementTree as etree
      print("running with ElementTree on Python 2.5+")
    except ImportError:
      try:
        # normal cElementTree install
        import cElementTree as etree
        print("running with cElementTree")
      except ImportError:
        try:
          # normal ElementTree install
          import elementtree.ElementTree as etree
          print("running with ElementTree")
        except ImportError:
          print("Failed to import ElementTree from any known place")

import json      
import glob
import os
          

styleSheet = """
  <property name="styleSheet">
   <string notr="true">background-color: %s;</string>
  </property>
  """
styleProperty = """
        <property name="%(propertyName)s">
         <color>
          <red>%(red)d</red>
          <green>%(green)d</green>
          <blue>%(blue)d</blue>
         </color>
        </property>
"""
colorMode = """
        <property name="colorMode">
         <enum>%(widgetClass)s::Static</enum>
        </property>
        """

##styleProperties = ['background', 'bordercolor', 'falseColor', 'trueColor', 'undefinedColor']
styleProperties = []

def listStyleProperties(styles):
    for key,values in styles.items():
        for style in values:
            try:
                styleProperties.index(style.encode())
            except ValueError:
                if style.find("colorMode") < 0 and style.find("background-color") < 0:
                    styleProperties.append(style.encode())
    print styleProperties
    return

def removeStyles(widget, indent):
    indent = indent.replace("|", " ")
    indent = indent.replace("-", " ")
    indent += "  "
    for prop in widget:
        propName = prop.get('name')
        try:
            propIndex = styleProperties.index(propName)
        except ValueError:
            propIndex = -1
        if (propName=='styleSheet' or propIndex >= 0 or propName=='colorMode'):
            print indent + "Removing " + propName
            prop.getparent().remove(prop)
    return

def addStyle(widget, style, indent):
    indent = indent.replace("|", " ")
    indent = indent.replace("-", " ")
    indent += "  "
    if style.has_key('background-color'):    #we need to add styleSheet
        print indent + "Adding styleSheet"
        widget.append(etree.fromstring(styleSheet % style.get('background-color')))
        return
    # we will be adding style properties instead
    for prop in styleProperties:    
        if style.has_key(prop):
            print indent + "Adding " + prop
            widget.append(etree.fromstring(styleProperty % {"propertyName": prop, "red": style.get(prop).get('red'), "green": style.get(prop).get('green'), "blue": style.get(prop).get('blue')}))
    if style.has_key('colorMode'):
        print indent + "Adding colorMode"
        widget.append(etree.fromstring(colorMode % {"widgetClass": style.get('colorMode')}))
        
    return

def replacer(widgets, indent):
    if len(widgets)>0:
        for widget in widgets:
            widgetClass = widget.get('class')
            if styles.has_key(widgetClass):
                print indent + "class: " + widgetClass + ' ' + "\tname: " + widget.get('name')
                removeStyles(widget, indent)
                addStyle(widget, styles.get(widgetClass), indent)
            if indent[-1] == '|':
                indent += "--"
            else:
                indent += "||"
            replacer(widget.findall('widget'), indent)
            replacer(widget.findall('layout'), indent)
            replacer(widget.findall('item'), indent)
    return

##########
## MAIN ##
##########
styles = json.load(open('styles.json', 'r'))
listStyleProperties(styles)

folders = glob.glob('*/')
try:
    folders.remove('styled/')
except ValueError:
    print "Output directory does not yet exist, so I will make one"

if len(folders) > 0:
    try:
        os.mkdir('styled')
    except OSError:
        print "Output directory allready exists"

for folder in folders:  
    files = glob.glob(folder + "*.ui")
    if len(files) > 0:
        try:
            os.mkdir('styled/'+folder)
        except OSError:
            print "Output directory for %s allready exists" % folder
        
    for uiFile in files:
        print "Working on " + uiFile
        f = open(uiFile, 'r');
        tree = etree.parse(f);
        root = tree.getroot();
        f.close()
        
        replacer(root, "--")
            
        fout = open('styled/'+uiFile, 'w')
        fout.write(etree.tostring(root))
        fout.close()
print "Done"