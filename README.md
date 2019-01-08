risc-v_decode_generator: Generate a VHDL 32-bit RISC-V ISA instruction decoder
==============================================================================

(c) 2019 Mike Field <hamster@snap.net.nz>

The input file contains the binary patterns for the instructions, and what flags will be asserted by the decoder when that pattern is seen.

All instructions must have a "type_" flag, and then zero of more "field_" flags.

"type_" flags are only asserted when a matching instruction is seen. This is also true of any non-"field_" flag

Do not act on "field_" flags unless the assocated "type_" flag is asserted(e.g. they select a subfunction)!

As a limited example (this is hust the load instructions), this input:

    Flags:
    -----------------000-----0000011 = valid, type_load,  flag_load_byte                   # OPCODE lbu
    -----------------001-----0000011 = valid, type_load,  flag_load_half                   # OPCODE lhu
    -----------------010-----0000011 = valid, type_load,  flag_load_word                   # OPCODE lw
    -----------------100-----0000011 = valid, type_load,  flag_load_byte, flag_load_extend # OPCODE lb
    -----------------101-----0000011 = valid, type_load,  flag_load_half, flag_load_extend # OPCODE lh

Generates this VHDL:

    LIBRARY ieee;
    USE ieee.std_logic_1164.ALL;
    
    ENTITY decoder IS
        PORT (
            inst                 : in  std_logic_vector(31 downto 0);
            flag_load_byte       : out std_logic := '0';
            flag_load_extend     : out std_logic := '0';
            flag_load_half       : out std_logic := '0';
            flag_load_word       : out std_logic := '0';
            type_load            : out std_logic := '0';
            valid                : out std_logic := '0');
    END ENTITY;
    
    ARCHITECTURE auto_generated_code OF decoder IS
    BEGIN
    
    flags_decode: PROCESS(inst)
        BEGIN
            flag_load_byte <= '0';
            IF ((inst(13 DOWNTO 12) = "00")) THEN
                flag_load_byte <= '1';
            END IF;
    
            flag_load_extend <= '0';
            IF ((inst(14 DOWNTO 14) = "1")) THEN
                flag_load_extend <= '1';
            END IF;
    
            flag_load_half <= '0';
            IF ((inst(12 DOWNTO 12) = "1")) THEN
                flag_load_half <= '1';
            END IF;
    
            flag_load_word <= '0';
            IF ((inst(13 DOWNTO 13) = "1")) THEN
                flag_load_word <= '1';
            END IF;
    
            type_load <= '0';
            IF ((inst(13 DOWNTO 13) = "0") AND (inst(6 DOWNTO 0) = "0000011")) THEN
                type_load <= '1';
            END IF;
            IF ((inst(14 DOWNTO 12) = "010") AND (inst(6 DOWNTO 0) = "0000011")) THEN
                type_load <= '1';
            END IF;
    
            valid <= '0';
            IF ((inst(13 DOWNTO 13) = "0") AND (inst(6 DOWNTO 0) = "0000011")) THEN
                valid <= '1';
            END IF;
            IF ((inst(14 DOWNTO 12) = "010") AND (inst(6 DOWNTO 0) = "0000011")) THEN
                valid <= '1';
            END IF;
    
        END PROCESS;
    END auto_generated_code;
