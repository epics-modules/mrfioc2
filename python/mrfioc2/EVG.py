# -*- coding: utf-8 -*-

import math

from fractions import Fraction, gcd

def lcm(numbers):
    return reduce(lambda x, y: (x*y)/gcd(x,y), numbers, 1)

class Event(object):
    def __init__(self, num, ref=None, delay=0, name=None):
        delay = int(delay)
        if ref and delay==0:
            raise ValueError("Event delay from reference must be non-zero")
        self.num, self.ref, self.delay = num, ref, delay
        self.name = name or ("#%d"%num)
        self.refby = set()

    def __str__(self):
        return self.name
    __repr__ = __str__

class Sequence(object):
    def __init__(self, gen, start, period, name="", events=[], omit=set(), include=set()):
        self.gen, self.name = gen, name
        self.start, self.period, self.events = start, period, dict(events)
        self.include, self.omit = set(include), set(omit)
        valset = set(self.events.itervalues())
        if len(valset)!=len(self.events):
            raise ValueError("Sequence root events must have unique times")
#        self.__valid = False

    @classmethod
    def join(cls, seqs, *args, **kws):
        # Check for overlaps (same event appears in more than one sequence)
        resultset = set()
        events={}
        if len(seqs)==0:
            raise ValueError("Can't join an empty list")
        evg = seqs[0].gen
        for S in seqs:
            if S.gen is not evg:
                raise ValueError("Can't mix Events from two EVGs")
            events.update(S.events)
            S = set(S.events.iterkeys())
            O = set.intersection(resultset, S)
            if O:
                raise ValueError("Sequences overlap: %s"%O)
            resultset.update(S)
        return Sequence(evg, events=events, *args, **kws)

    def _validate(self):
#        if self.__valid:
#            return
        self.gen._validate()
        events = {}
        for E,T in self.events.iteritems():
            T = T%self.period
            if E in self.gen.events:
                events[E] = T
            elif E in self.gen.num2event:
                events[self.gen.num2event[E]] = T
            else:
                raise ValueError("Sequence references unknown event %s"%E)

        for E in events.iterkeys():
            # Already checked for cycles, so we know the loop will terminate
            while E.ref:
                if E.ref in events:
                    raise ValueError("Sequence references event without unique time: %s"%E.ref)
                E = E.ref

        self.events = events
#        self.__valid = True

    def expand(self, debug=True):
        """Return a sequence with relative delays translated to absolute
        """
        self._validate()

        # breadth first traversal of the back reference tree

        # pre-populate with reference (absolute) times
        result = dict([(E,T+self.start) for (E,T) in self.events.iteritems()])
        if debug:
            print 'Start with',result
        # start traverse with 2nd level events.
        todo = set.union(*[E.refby for E in result])

        while todo:
            E = todo.pop()
            assert E not in result
            result[E] = (result[E.ref] + E.delay)%self.period
            if debug:
                print '%s = %s + %d = %d'%(E, E.ref, E.delay, result[E])
            todo.update(E.refby)

        result = result.items() # to list
        
        if self.include:
            result = filter(lambda (E,T):E in self.include or E.num in self.include, result)
        if self.omit:
            result = filter(lambda (E,T):E not in self.omit and E.num not in self.omit, result)

        result.sort(key=lambda I:I[1]) # sort by increasing time
        prev = None
        for E,T in result:
            if not prev:
                prev = E,T
                continue
            if T==prev[1]:
                raise ValueError("Resulting sequence contains colliding Events %s and %s at %d"%(E,prev[0],T))
            prev = E,T
        return result

class EVG(object):
    def __init__(self, rffreq, div):
        self.rf, self.div = rffreq, div
        self.freq, self.period = rffreq/div, div/rffreq
        self.num2event = {} # Map numbers to Event()s (not always unique!)
        self.events = set() # all Event()s
        self.seqs = set()
        self.__valid = True

    def mxcPV(self,**kws):
        return "%(SYS)s{%(D)s-Mxc:%(N)s}Prescaler-SP"%kws

    def seqCodesPV(self,**kws):
        return "%(P)sEvtCode-SP"%kws

    def seqTimesPV(self,**kws):
        return "%(P)sTimestamp-SP"%kws

    def ticks(self, sec):
        # rounds down
        return int(sec*self.freq)

    def sec(self, tick):
        return "%.09f"%(tick/self.freq)

    def event(self, num, **kws):
        if num in self.events:
            raise ValueError("Event code %d already allocated"%num)
        E = Event(num, **kws)
        self.num2event[num] = E
        self.events.add(E)
        self.__valid = False
        return E

    def seq(self, *args, **kws):
        S = Sequence(self, *args, **kws)
        self.seqs.add(S)
        return S

    def _validate(self):
        """Validate all events, look for loops etc.
        """
        if self.__valid:
            return

        # Clear previous
        for E in self.events:
            E.refby = set()

        # Catalog back references
        for E in self.events:
            if not E.ref:
                continue

            if E.ref in self.events: # ref with instance
                E1 = E.ref
            elif E.ref in self.num2event: # ref with number
                E.ref = self.num2event[E.ref]
                E1 = E.ref
            else:
                raise ValueError("%s references unknown events %s"%(E, E.ref))

            E1.refby.add(E)

        # Look for loops
        for E in self.events:
            Es = []
            while E.ref:
                if E.ref in Es:
                    Es.append(E.ref)
                    raise ValueError("Event reference loop detected: %s"%(' -> '.join(Es)))
                Es.append(E.ref)
                E = E.ref

        self.__valid = True

    def findDiv(self, outfreq, ref=None):
        """Find the divider on the EVG frequency which produces an actual output
        frequency less than or equal to the requested output frequency
        """
        return math.ceil((ref or self.freq)/outfreq)


    def findCommonDiv(self, startfreq=None, others=[], ref=None):
        """Find the lowest integer divider on the EVG frequency which is also an integer
        divider for the other frequencies specified.  The other frequences
        are given as (possibly fractonal) multipliers on the EVG frequency.

        The start and stop frequency bound the search.
        """

        # Find divider which will produce the high frequency which satisfy constraints
        N = int(lcm([1/Fraction(o) for o in others]))
        
        if not startfreq:
            return N
        
        # Find the range of possible EVG dividers
        Nmin = self.findDiv(startfreq, ref=ref)

        # Find smallest integer x such that
        # Nmin <= N*x

        Nx = int(math.ceil(Nmin/N))*N

        assert (ref or self.freq)/Nx <= startfreq

        return Nx
