--------------------------------------
N64 Protocol and hardware observations
--------------------------------------

The existing resources on the 'net weren't quite enough
to write a successful emulation of an N64 controller.
The following are some random notes from my own reverse-engineering.

Command summary:
  00: identify, returns 05 00 02 with no controller pak,
                05 00 01 with a memory or rumble pak

  01: status, returns a 24-bit button and axis status packet

  02: read from expansion bus, followed by 2 bytes (always 80 01)
      from the N64.

  03: write to expansion bus. Followed by 2 bytes (always 80 01)
      then 32 bytes of data.

Expansion bus pinout, numbered according to the silkscreen markings
on the controller's PCB:

  Pin   Name      Note
-----------------------
  1     Ground
  2     A14       (1)
  3     A12
  4     A7
  5     A6
  6     A5
  7     A4
  8     A3
  9     A2
  10    A1
  11    A0
  12    D0
  13    D1
  14    Detect    (2)
  15    3.3v
  16    D2

  17    Ground
  18    Unknown 1 (3)
  19    A15
  20    /WE
  21    A13
  22    A8
  23    A9
  24    A11
  25    /OE
  26    A10
  27    D7
  28    D6
  29    D5
  30    D4
  31    3.3v
  32    D3

 Note 1:
   With 15 address lines, that gives standard memory paks a 32k capacity.
   The Nyko Hyperpak tested has 128k of SRAM, but includes a switch that
   selects between four 32k banks.

 Note 2:
   There's a 3k resistor between detect and +3.3v in the controller pak.
   When this signal is high, the controller thinks a controller pak is
   present- it will attempt to read and write from it, and it acknowledges
   to the N64 with an 0xE1. When this signal is low, the controller doesn't
   attempt any writes or reads and it acknowledges with 0x1E.

 Note 3:
   Possibly a chip select


Majora's Mask observed startup sequence

	1. N64 sends identify command, 0x00
	   Controller responds with 05 00 02 with no controller pak,
	   05 00 01 with memory pak.

	2. N64 reads controller status (0x01)

	3. Another 0x00 command, with the same response

	4. N64 sends:
	   03 80 01 followed by 32 bytes, all FE
	   Response: 0xE1 with memory pak, 0x1E without.
	   The response has no stop bit! instead, the data line
	   goes low for 2us immediately after the last data bit.

Majora's Mask title screen polling sequence

	1. Status request (0x01)

	2. Identification command (0x00)

	3. Another 0x03 command as described above

	4. N64 sends:
	   02 80 01


Hooked up the controller pak to the logic analyzer. Looks like 0x03 is
indeed a write, and the 32 bytes are all data directed at the controller
pak bus. The two preceeding bytes probably indicate the address. At least
one of the unknown pins in the pinout are probably high bits in the address,
not used by the RAM but maybe used by the rumble pak or other controller paks.

Saved trace "WR1" with rumble pak attached, during game play in Super Smash.
Can't make out a whole lot, but it's clear how the 0x03 packets work.

Recorded an odd packet at the title screen of Majora's Mask- controller slot
was empty, but I was pulling the DETECT pin high to get it to talk anyway.
N64 sent a write (03 80 01) then 32 0x80 bytes. Controller responded with
0xB8 (!). Saved to "no_pak".

The address does auto-increment during a 32-byte write to the controller pak.
With the write command 03 80 01, addresses start at zero.

Memory pak reads, observed in Bomberman 64:
N64 sent: 02 00 35
Read 32 bytes starting at 0x0020.
Trace saved as "RD1".

Command    Start address
-------------------------
02 80 01   0000
02 00 35   0020
02 01 16   0100
02 01 23   0120
02 01 49   0140
02 01 7C   0160
02 01 9D   0180

Rumble pak writes:
To switch on the rumble pak motor, the N64 sends:
03 C0 1B 01 01 01 ...
This writes 01 to addresses starting at 0x4000.
To turn the motor back off, the N64 sends:
03 C0 1B 00 00 00 ...

So, it looks like the rumble pak is a simple memory
mapped latch accessed using the same protocol as the
memory pak.

But how does the N64 know which peripheral is attached?
Initialization, from Super Smash Bros:

N64 sends 03 80 01 then 32 0xFE bytes, controller writes
them starting at address 0x0000.
N64 then starts a read using 02 80 01.
This seems to verify that the address bytes in the read
and write commands use the same format!

The rumble pak returns all 0x80 from this read, while
a memory pak will return 0x00. Why doesn't the memory
pak return 0xFE?

It looks like the status byte returned from a controller
pak write is actually a checksum. This would make a lot of
sense, as when saving a game data integrity is important.
When generating write commands with a constant 80 01 address
word and changing data, the checksum byte changes. Several
messages were tested consisting of 80, 01, 31 zero bytes,
then a different byte. The byte and the message's checksums
are recorded below:

byte   message checksum
-------------------------
00     FF  1111 1111 <-- seems to indicate that the checksum is only
01     7A  0111 1010     over the 32 bytes, not the address word
02     70  0111 0000
03     F5  1111 0101
05     E1  1110 0001
06     14  0001 0100
07     6E  0110 1110
C5     EE  1110 1110
FF     72  0111 0010

01     7A  0111 1010
02     70  0111 0000
04     64  0110 0100
08     4C  0100 1100
10     1C  0001 1100



- Memory map

0x0000 - 0x7FFF :  SRAM, on the memory pak

0x8000          :  Identification/initialization.
                   Write all 0xFEs to it for initialization, then
                   read back 0x00 on the memory pak or 0x80 on the rumble pak.

0xC000          :  Motor control latch (low bit) for the rumble pak


- Figured out a lot more about the detection sequence.. see bus.py for
  an implementation of it, with comments

Poked around in the saved game format a little.. bomberman's saved characters
are easy to modify, it uses a very simple checksum and each character attribute
is just a byte.

The game names used in the controller pak data screen (press start while turning
on the N64) aren't ASCII, but the character set is simple enough. Here's a partial
translation table:

"ABCDEFGHIJKLMNOPQRSTUVWXYZ "
"\x1a\x1b\x1c\x1d\x1e\x1f !\"#$%&'()*+,-./0123\x0F"

Each game gets a 32-byte table of contents entry, starting at 0x300. The second
half of each entry is reserved for a 16-byte zero-padded game name.
