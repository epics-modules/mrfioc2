# -*- coding: utf-8 -*-

def caput(pv, value):
    if isinstance(value, str):
        scalar = True
    else:
        try:
            len(value)
            scalar = False
        except TypeError:
            scalar = True

    if scalar:
        print "caput",pv,value
    else:
        print "caput -a",pv,len(value),(" ".join(map(str,value)))
