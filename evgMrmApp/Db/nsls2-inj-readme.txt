NSLS2 Injection Sequence Constructor

Note: nsls2-inj-modes.db was constructed with VDCT.  It also contains info() tag entrys
      which will be lost when edited with VDCT <2.6.1278.

Injector timing modes are represented by a 14-bit unsigned integer.

Each bit indicates whether a specific time slot in a specific sequence is populated.

Bit # - Slot
0 - Only slot in 1Hz BR (0x0001)
1 - Only slot in Stacked BR (x0002)
2,3 - Slots in 2Hz BR.  Bit 2 is earlier (0x000C)
4-13 - Linac slots.  Bist 4 is earliest  (0x3FF0)

Two software sequences are constructed.  One without "beam" events, and one with "beam" events.  The "beam" events being those associated with the electron source, and the beam diagnostics.  The other non-beam events are everything else, including ramping power supplies and RF systems which should always run to maintain equilibrium.

The Single Shot Controller (SSC) is the logic which selects which sequence to run next.  Both sequencers should be configured with Run Mode as Single.

Files:

nsls2-inj-seqs.substitutions  = Brings together the following pieces

nsls2-inj-calc.db  = Calculates some relative ratios

nsls2-inj-modes.db  = Logic to translate (and validate) operator sequence selections into bit masks for the sequence repeater

nsls2-inj-ssc.db = Single shot controller

seq-repeater.db  = An aSub record which takes short sequence and repeats it with time delay(s).

seq-merger.db  = An aSub record which takes number of sequences and merges them into single sequences.  Inputs must be sorted.  Sorting is preserved in the output.


Usage example.

 dbLoadRecords("db/vme-evg230-nsls2.db", "SYS=INJ-TS, D=evg:0, EVG=EVG1")

 dbLoadRecords("db/nsls2-inj-seqs.db","LN=LN-TS, BR=BR-TS, INJ=INJ-TS, EVG=evg:0, SEQN=SoftSeq:InjN, SEQB=SoftSeq:Inj")

The NSLS2 EVG database provides soft sequences named "SoftSeq:InjN" (w/o beam) and "SoftSeq:1" (all events).
