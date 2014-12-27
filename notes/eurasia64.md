Nintendo 64 Controller FAQ V0.2
Written By $up3rDud3

All Copyrights and Trademarks belong to their perspective owners.

This work was done via reverse engineering the controller itself.
No information was obtained via outside channels. All work was done on
a N64 controller with no prior knowledge of its working. The hope was to
interface the controller with a common pc game interface. Now working on
full ISA interface.

It was deduced that it is a serial type interface, and the other two wires were Power
and ground. So once you take the thing apart, you see the main controller
wire going to a plug on the main board. These three wires are color coded as
follows

	Red - 3.3V
	Black - Ground
	White - Data : 3.3V Logic Level

Looking at the connector end that goes into the N64 we see

	  ____
	 /         <- Round Part is Top
	/       
	||       ||
	|| O O O ||
	||_____||

	  1 2 3

	1 - GND
	2 - DATA - 3.3V Logic Level
	3 - VCC - 3.3V



1.0	Reading the controllers position and button information

For reading position information from controller, the data stream requires a 41 bit
command sequence. To understand the sequence we must define the bit stream characteristics.

	Each bit is 4us in length (except 2)
	A zero is represented as 3us low (0V) and 1us high (3.3V).
	A one is represented as 1us low (0V) and 3us high (3.3V).

For the read status command 7 zeros and 2 ones must be sent to the controller.

[http://www.eurasia.nu/images/submitted/n64_joystick_read_command.gif]

The way the transmitter should be implemented is using tristate logic. For a zero tie
the input of a tristate buffer low and use the tristate control pin as the data input.
The nintendo controller has a 3.3V pull up on the line so if you put the buffer in
tristate the data line will go to 3.3V (high). If you put the buffer in drive mode the
line will go to 0V (low). This also allows you to go directly to read mode in your logic
once you send the last command bit.

For reading the controller, once the last bit goes to the high state, approximately 2us later the controller
starts sending the requested data back to you, so make sure your receiver is ready.

....Comming soon full isa schematic==


1.1	Bit assignments

The 32 bits following the start bit are translated as follows

bits 0 - 15 : button values -
                             ||
                              -> Zero = Not Pressed
                             ||
                              -> One = Pressed

bits 16 - 31 : analog stick position

bit        0 - A
           1 - B
           2 - Z
           3 - START
           4 - DIGITAL PAD UP
           5 - DIGITAL PAD DOWN
           6 - DIGITAL PAD LEFT
           7 - DIGITAL PAD RIGHT
           8 - START+L+R : All pressed at the same time
           9 - ? No combination seems to use this bit
          10 - L
          11 - R
          12 - C UP
          13 - C DOWN
          14 - C LEFT
          15 - C RIGHT

bit       16 -> 23 - X axis reading, 2's compliment
                     -128 = full left
                        0 = center
                     +128 = full right

bit       24 -> 31 - Y axis reading, 2's compliment
                     +128 = full top
                        0 = center
                     -128 = full bottom

1.2	Timing Information

The total position command takes 166us to execute, and the program I ran on the N64 polled the
controller every 16.667ms or a frequency of 60Hz.

1.3	Rumble Pack Commands

It appears as if the rumble pack commands are more difficult to decode when your trying to play
while hitting stop on the oscilloscope. So as far as I can tell until I get the controller
hooked up to a PC this is the format for the rumble pack commands. Use the same data format for
sending a 0 and a 1 bit.

	0000-0011-1100-0000-0001-1011-(Send string of zeros the length of intensity required)-(6us pulse)-0000-0000

This is probably wrong, but oh well.

1.4	Memory Pack Commands

?????



---==_+_' N64 Joystick Port Pinout _'+_==---
                        By CrowKiller

Pinout coming from a memory pack, pin names are
in relation to a standard SRAM chip.

From top of memory pack
         (front side)

       16             1
          ==
       17             32
          (back side)

Side 1
01- I/O 02
02- Voltage in (apprx. +3.5 volts)
03- ???online??? (it seems like an output)
04- I/O 01
05- I/O 00
06- ADR 00
07- ADR 01
08- ADR 02
09- ADR 03
11- ADR 05
12- ADR 06
13- ADR 07
14- ADR 12
15- ADR 14
16- GND

Side 2
17- GND
18- CE  (trigger = high)
19- CE  (trigger=low voltage)
20- WE  (trigger=low voltage)
21- ADR 13
22- ADR 08
23- ADR 09
24- ADR 11
25- OE  (trigger=low voltage)
26- ADR 10
27- I/O 07
28- I/O 06
29- I/O 05
30- I/O 04
31- Voltage in (apprx. +3.5 volts)
32- I/O 03