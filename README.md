== Generate a 32-bit RISC-V ISA instruction decoder ==

The input file contains the binary patterns for the instructions, and what flags will be asserted by the decoder when that pattern is seen.

All instructions must have a "type_" flag, and then zero of more "field_" flags.

"type_" flags are only asserted when a matching instruction is seen. This is also true of any non-"field_" flag

Do not act on "field_" flags unless the assocated "type_" flag is asserted(e.g. they select a subfunction)!
