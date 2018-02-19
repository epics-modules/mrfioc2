#!/usr/bin/env python
"""Automatic test of an EVG and EVR.

Event clocks must be configured prior to running script.


run as:
  python selftest.py <SYS> <EVG_D> <EVR_D>

eg.
  python selftest.py TST evg:1 evr:5

when loaded as:
  dbLoadRecords("db/evr-vmerf-230.db","SYS=TST, D=evr:5, EVR=EVR1")
  dbLoadRecords("db/vme-evg230.db","SYS=TST, D=evg:1, EVG=EVG")
"""
import logging
log = logging.getLogger(__name__)

from random import randrange

import unittest, sys, time

from cothread.catools import caget, caput as _caput

class MRFTest(unittest.TestCase):
    def evr(self, tmpl):
        return str(tmpl%(args.sys, args.evr))
    def evg(self, tmpl):
        return str(tmpl%(args.sys, args.evg))

    def assertPVEqual(self, pv, expect, **kws):
        actual = caget(pv, **kws)
        self.assertEqual(actual, expect)

    def test_soft_event(self):
        """Send a software event and see that the EVR receives it
        """
        prev = caget(self.evr('%s{%s}EvtACnt-I'))
        caput(self.evg('%s{%s-SoftEvt}EvtCode-SP'), 10)
        self.assertPVEqual(self.evr('%s{%s}EvtACnt-I'), prev+1)

    def load_seq(self, seq):
        dly, code = [], []
        for D, C in seq:
            dly.append(D)
            code.append(C)

        caput(self.evg('%s{%s-SoftSeq:0}EvtCode-SP'), code)
        caput(self.evg('%s{%s-SoftSeq:0}Timestamp-SP'), dly)
        caput(self.evg('%s{%s-SoftSeq:0}Commit-Cmd'), 1)

    def do_test_sequence(self, single=True):
        """Setup a software triggered sequencer in Single mode
        """
        # watch EVR counter to ensure EVR FIFO IRQ works
        prev11 = caget(self.evr('%s{%s}EvtBCnt-I'))
        prev12 = caget(self.evr('%s{%s}EvtCCnt-I'))
        # watch EVG counter to ensure that EVG start/end of seqeunce IRQ works
        starts = caget(self.evg('%s{%s-SoftSeq:0}NumOfStarts-I'))
        ends = caget(self.evg('%s{%s-SoftSeq:0}NumOfRuns-I'))

        caput(self.evg('%s{%s-SoftSeq:0}RunMode-Sel'), 'Single' if single else 'Normal')
        caput(self.evg('%s{%s-SoftSeq:0}TrigSrc:0-Sel'), 'Software')
        caput(self.evg('%s{%s-SoftSeq:0}TsResolution-Sel'), 'Ticks')
        self.load_seq([
            (0, 11),
            (1, 12),
        ]) # commits
        caput(self.evg('%s{%s-SoftSeq:0}Load-Cmd'), 1)
        caput(self.evg('%s{%s-SoftSeq:0}Enable-Cmd'), 1)

        self.assertPVEqual(self.evr('%s{%s}EvtBCnt-I'), prev11)
        self.assertPVEqual(self.evr('%s{%s}EvtCCnt-I'), prev12)
        self.assertPVEqual(self.evg('%s{%s-SoftSeq:0}NumOfStarts-I'), starts)
        self.assertPVEqual(self.evg('%s{%s-SoftSeq:0}NumOfRuns-I'), ends)

        caput(self.evg('%s{%s-SoftSeq:0}SoftTrig-Cmd'), 1)
        time.sleep(0.1)

        self.assertPVEqual(self.evr('%s{%s}EvtBCnt-I'), prev11+1)
        self.assertPVEqual(self.evr('%s{%s}EvtCCnt-I'), prev12+1)
        self.assertPVEqual(self.evg('%s{%s-SoftSeq:0}NumOfStarts-I'), starts+1)
        self.assertPVEqual(self.evg('%s{%s-SoftSeq:0}NumOfRuns-I'), ends+1)

        # When Single mode, so now disabled
        self.assertPVEqual(self.evg('%s{%s-SoftSeq:0}Enable-RB'), 0 if single else 1)

        # try to trigger again, no-op in Single
        caput(self.evg('%s{%s-SoftSeq:0}SoftTrig-Cmd'), 1)

        self.assertPVEqual(self.evr('%s{%s}EvtBCnt-I'), prev11+1 if single else prev11+2)
        self.assertPVEqual(self.evr('%s{%s}EvtCCnt-I'), prev12+1 if single else prev12+2)

    def test_single_sequence(self):
        self.do_test_sequence(single=True)

    def test_normal_sequence(self):
        self.do_test_sequence(single=False)

    def test_dbuf8(self):
        dlen = randrange(1,3)*4
        expect = [randrange(0, 255) for i in range(dlen)]

        caput(self.evg('%s{%s}dbus:send:s8'), expect)
        time.sleep(0.1)
        actual = list(caget(self.evr('%s{%s}dbus:recv:s8')))
        self.assertListEqual(actual, expect)

    def test_dbuf32(self):
        dlen = randrange(1,3)*4
        expect = [randrange(0, 0xffffffff) for i in range(dlen)]

        caput(self.evg('%s{%s}dbus:send:u32'), expect)
        time.sleep(0.1)
        actual = list(caget(self.evr('%s{%s}dbus:recv:u32')))
        self.assertListEqual(actual, expect)

def caput(*args, **kws):
    K = {'wait':True}
    K.update(kws)
    _caput(*args, **K)

def getargs():
    from argparse import ArgumentParser
    P = ArgumentParser()
    P.add_argument('sys', help='common SYS prefix')
    P.add_argument('evg', help='EVG D name')
    P.add_argument('evr', help='EVR D name')
    P.add_argument('extra', nargs='*', default=[])
    return P.parse_args()

def main(args):
    Gclk = caget('%s{%s-EvtClk}Frequency-RB'%(args.sys, args.evg))
    Rclk = caget('%s{%s}Link:Clk-I'%(args.sys, args.evr))
    diff = (Rclk - Gclk)/Gclk
    if diff>0.1:
        raise RuntimeError("EVR (%s) and EVG (%s) clocks differ"%(Rclk, Gclk))
    link = caget('%s{%s}Link-Sts'%(args.sys, args.evr))
    if link!=1:
        raise RuntimeError('EVR link not OK (%s)'%link)

    sys.argv = sys.argv[:1] + args.extra
    unittest.main()

if __name__=='__main__':
    args = getargs()
    logging.basicConfig(level=logging.DEBUG)
    main(args)

