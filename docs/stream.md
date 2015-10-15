# Stream description for SSpMV

Given: A_s, v_s, u_s submatrices and subvectors on processors.
Construct: stream containing SSpMV info

# Options: horizontal or vertical strips

Horizontal strips: can compute sums *sum_s u_is* on cores. Need to
add a second stream with vector data, rendering communication between cores
obsolete but slowing down the process in general.

Vertical strips: compute many partial sums u_is, but only one per strip. vector
*can make unique per window*, those who own u_is are responsible for adding to
previous strip?
data only has to be pulled in once for an entire strip, local communication
is fast.

## Definition of SSpMV stream (per core):

..........................................................

### global header
maxSizeU         int
maxSizeV         int
maxSizeWindow    int
maxNonLocal      int
numStrips        int

(repeat numStrips times):
### strip header
numWindows       int
numLocalV        int
v                int[numLocalV]

(repeat numWindows times):
### window
numNonLocal      int
nonLocalOwners   int[numNonLocal]
nonLocalIdxs     int[numNonLocal]
windowSize       int
tripletRows      int[windowSize]
tripletCols      int[windowSize]
tripletVals      float[windowSize]

..........................................................

## Local storage

- A in use
- A next
- remote owners
- remote indices
- v local
- v from remote
- u (always local)
