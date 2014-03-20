# -*- coding: utf-8 -*-

from cothread import catools as ca

def caput(pv, value):

    cval = ca.caget(pv)
    if getattr(cval,'__len__',False) or getattr(value,'__len__',False):
        cval = map(int, cval)
        if len(cval)!=len(value):
            print pv,'Length mismatch'
        elif all([a==b for (a,b) in zip(cval,value)]):
            print pv,'OK'
            return
        else:
            print pv,'Mismatch'

    else:
        if cval==value:
            print pv,'OK'
            return
        else:
            print pv,'Mismatch'

    print ' expect',value
    print ' found',cval
