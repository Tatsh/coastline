# Label macros of the assembly spimdisasm generates. Every label is global. A routine converted to
# C and the assembly still in use refer to each other by name.

.macro glabel label
    .global \label
    .type \label, @function
    \label:
.endm

.macro dlabel label
    .global \label
    .type \label, @object
    \label:
.endm

.macro jlabel label
    .global \label
    \label:
.endm

.macro alabel label
    .global \label
    \label:
.endm

.macro endlabel label
    .size \label, . - \label
.endm

.macro enddlabel label
    .size \label, . - \label
.endm

# The disassembler marks every routine and datum it could not match. The marker has no effect on
# the build.
.macro nonmatching label, size=1
.endm
