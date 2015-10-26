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

### chunk: global header
maxSizeU         `uint32_t`
maxSizeV         `uint32_t`
maxSizeWindow    `uint32_t`
maxNonLocal      `uint32_t`
numStrips        `uint32_t`

(repeat numStrips times):
### chunk: strip header
numWindows       `uint32_t`
numLocalV        `uint32_t`
v                `uint32_t[numLocalV]`

(repeat numWindows times):
### chunk: window
numNonLocal      `uint32_t`
nonLocalOwners   `uint32_t[numNonLocal]`
nonLocalIdxs     `uint32_t[numNonLocal]`
windowSize       `uint32_t`
tripletRows      `uint32_t[windowSize]`
tripletCols      `uint32_t[windowSize]`
tripletVals      `float[windowSize]`

..........................................................

## Local storage

- A in use
- A next
- remote owners
- remote indices
- v local
- v from remote
- u (always local)
