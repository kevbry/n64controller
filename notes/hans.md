The data exchange between the controller and the host:

The N64 controller has only 3 cables to connect to the N64 console. A red one for the power supply (Vcc +3,6V), a black one for ground and a white one for the data exchange.

The data exchange through the "data wire" is bidirectional and could be driven by an open collector device (as the N64 controller itself does).

In its idle state the data line is high ( 3,6 V).
Each bit that is transmitted takes 4 µs of time. When a bit is send to, or from the controller, it starts with pulling data to low (0 V). If the bit has the value "0", the data line remains 3 µs low and then rises to high for 1 µs. If the bit that should be send has the value "1", the data line remains only 1 µs low and rises high for 3 µs.

n64bits.gif (1234 Byte)

The N64 controller needs to be polled through the host.
So a "request sequence" must be send to the controller.

For the simple data polling (buttons and joystick), sending a 9 bit value : "000000011" to the controller is necessary. 
If someone has more informations about the commands that could be send to the controller, please inform Simon or me about it !!!

colorbar.gif (4491 Byte)

Immediately**) after the request sequence, the N64 controller starts to transmit its data.
It starts with the value for the "A" button, proceeds with the "B" button and transmits at last information the y-axis value for the analog stick:

**) unfortunately this "immediately" is not constant and causes a jitter to the incomming data.

bit	
data for:

1	A
2	B
3	Z
4	START
5	UP
6	DOWN
7	LEFT
8	RIGHT
9	unused
10	unuesd
11	L
12	R
13	C-UP
14	C-DOWN
15	C-LEFT
16	C-RIGHT
17-24	X-AXIS
25-32	Y-AXIS
33	stopbit
For each button a "1" represents a pressed button, and a "0" means: button released.
Now for the values of the analog stick:
The data seems to be appear in a  signed format. For the x-axis for example it looks like the follwing:

position:	bit 17	bit 18	bit 19	bit 20	bit 21	bit 22	bit 23	bit 24
left edge	0	1	1	1	1	1	1	1
......		0	x	x	x	x	x	x	x
 			0	0	0	0	0	0	1	1
 			0	0	0	0	0	0	1	0
left		0	0	0	0	0	0	0	1
middle		0	0	0	0	0	0	0	0
right		1	1	1	1	1	1	1	1
 			1	1	1	1	1	1	1	0
 			1	1	1	1	1	1	0	0
......		1	x	x	x	x	x	x	x
right edge	1	0	0	0	0	0	0	0
It seems that I can't reach the highest and the lowest values for the x- and y- axis with my N64 controller.

The same format is valid for the y-axis (obviously), where the corresponding data is transmitted from bit 23 to 32.

The stopbit is always "1".

If you count together the times for the polling sequence (9*4 µs) and the returning data (33*4 µs) you get 164 µs at all for the bidirectional data transmission sequence. If you now immediately send the next request sequence to the controller you will receive.... nothing. The controller needs approximately 200 further µs to be ready for the next poll cycle.

 

colorbar.gif (4491 Byte)

