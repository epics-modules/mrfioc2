#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys

from fractions import Fraction as F

from mrfioc2 import EVG

if sys.version_info[:3]<(2,7):
    from unittest import TestCase as _TestCase
    # Compatibility with python 2.7
    class TestCase(_TestCase):
        def assertDictEqual(self, A, B):
            return self.assertSequenceEqual(A.iteritems(), B.iteritems())
        def assertListEqual(self, A, B):
            return self.assertSequenceEqual(iter(A), iter(B))
        def assertSequenceEqual(self, A, B):
            from itertools import izip
            msg=[]
            for i, (iA, iB) in enumerate(izip(A,B)):
                if not (iA==iB):
                    msg.append('[%d](%s, %s)'%(i,iA,iB))
            if len(msg):
                msg = ' '.join(msg)
                self.fail('%s != %s : %s'%(A,B,msg))
        def assertIs(self, A, B):
            if A is not B:
                self.fail('%s is not %s'%(A,B))
        def assertAlmostEqual(self, A, B, delta=None, **kws):
            if delta is None:
                return _TestCase.assertAlmostEqual(self, A, B, **kws)
            if abs(B-A)>delta:
                self.fail('%s!=%s (diff=%g)'%(A,B,abs(B-A)))

else:
    from unittest import TestCase

class TestEVGBasic(TestCase):

    def test_basic(self):
        E = EVG.EVG(499.68e6, 4)
        self.assertAlmostEqual(E.freq, 124.92e6, delta=1e3)
        self.assertAlmostEqual(E.period, 8.005e-9, delta=1e-12)

        self.assertIs(E.ticks(32e-9), 3) # rounding down

        E._validate()

    def test_freq(self):
        E = EVG.EVG(499.68e6, 4)

        self.assertEqual(E.findDiv(62.5e6), 2)

        self.assertEqual(E.findDiv(10e3), 12492)

        self.assertEqual(E.findDiv(10), 12492000)
        self.assertEqual(E.findDiv(9.8), 12746939)

        self.assertEqual(E.findDiv(E.freq), 1)
        self.assertEqual(E.findDiv(E.freq/9999), 9999)

        self.assertEqual(E.findCommonDiv(10e3, []), 12492)

        self.assertEqual(E.findCommonDiv(10e3, [1]), 12492)

        # 124.92e6/12500 == 12.492e6/1250 == 9993.6
        self.assertEqual(E.findCommonDiv(10e3, [F(1,10)]), 12500)

        self.assertEqual(E.findCommonDiv(10.0, [F(3,14),F(1,12540),F(2,25)]), 12728100)


class TestEVGSeq(TestCase):

    def setUp(self):
        E = EVG.EVG(499.68e6, 4)

        es = []
        es.append(E.event(14))
        es.append(E.event(15))
        es.append(E.event(16, ref=es[1], delay=5))
        es.append(E.event(16, ref=es[1], delay=6))
        es.append(E.event(16, ref=es[2], delay=6))
        self.E, self.es = E, es

    def test_events(self):
        E, es = self.E, self.es

        E._validate()

        self.assertEqual(E.events, set(es))
        self.assertDictEqual(E.num2event, dict([(evt.num,evt) for evt in es]))

        self.assertIs(es[2].ref, es[1])
        self.assertEqual(es[1].refby, set(es[2:4]))
        self.assertEqual(es[2].refby, set(es[4:5]))

    def test_seq(self):
        E, es = self.E, self.es

        S1 = E.seq(1, 100, events=[(es[0],0)])

        S1x = S1.expand()
        self.assertListEqual(S1x, [(es[0],1)])

        S2 = E.seq(1, 100, events=[(es[0],0),(es[1],1)])

        S2x = S2.expand()
        self.assertListEqual(S2x, [(es[0],1),(es[1],2),(es[2],7),(es[3],8),(es[4],13)])

        S3 = E.seq(1, 10, events=[(es[0],0),(es[1],1)])

        S3x = S3.expand()
        self.assertListEqual(S3x, [(es[0],1),(es[1],2),(es[4],3),(es[2],7),(es[3],8)])

        S4 = E.seq(1, 100, events=[(es[1],1)])
        S5 = S4.join([S4,S1], 1, 100)

        self.assertListEqual(S5.expand(), S2x)

    def test_bad(self):
        E, es = self.E, self.es

        # A zero delay will always overlap
        self.assertRaises(ValueError, E.event, 17, ref=es[1], delay=0)

        # sequence with two starting events which will always overlap
        self.assertRaises(ValueError, E.seq, 1, 100, events=[(es[0],0),(es[1],0)])

        SX = E.seq(1, 100, events=[(es[0],7),(es[1],1)])

        # calculated sequence contains an overlap between es[0] and es[2]
        self.assertRaises(ValueError, SX.expand)

        self.assertRaises(ValueError, SX.join, [SX,SX], 1, 100)

if __name__=='__main__':
    import unittest
    unittest.main()
