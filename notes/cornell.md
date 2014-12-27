##Controller Interface


The N64 controller is a serial device with three pins: Ground, Vcc, and a bidirectional data line. Most of the published documentation states that Vcc is set to somewhere between 3.3 and 3.6 volts. We found that the controller is fairly insensitive and operated between 3.0 and 3.8 volts.

Data is encoded in a format peculiar to nintendo. The beginning of every bit is signified by a falling edge and is 4 microseconds in duration. A 0 is asserted if the line is pulled low for 3 microseconds, then left high for the last 1 microsecond. If, however, the line is only pulled low for 1 microsecond, a 1 is asserted.

The poll request is a string of seven 0's and two 1's. A short time later, about 2 microseconds with significant jitter, the contrller returns 32 status bits and 1 stop bit. The first 16 bits show the status of the digital buttons. The last 16 bits are two 8 bit 2's-complement segments showing the position of the analog stick along the X and Y axis. The controller then enters a 'quiet' state for about 200 microseconds where it will not respond to further poll requests.

##N64 Bit Sequence

Bit	0	1	2	3	4	5	6	7
Button	A	B	Z	Start	D-Up	D-Down	D-Left	D-Right
 
Button	8	9	10	11	12	13	14	15
Bit	unused	unused	L	R	C-Up	C-Down	C-Left	C-Right
Example Control Stick Values

Left Extreme Edge	0	1	1	1	1	1	1	1
Left Off Center		0	0	0	0	0	0	0	1
Center				0	0	0	0	0	0	0	0
Right Off Center	1	1	1	1	1	1	1	1
Right Extreme Edge	1	0	0	0	0	0	0	0

The standard class chip, the Atmel 90s8515, was fast enough at 8 Mhz to write and read on a 1 microsecond time scale, but it didn't leave much time for other necessary instructions. I decided to go with the Atmel 90s1200, knowing it could be clocked at 20 Mhz.

Since the 1200 has no RAM, 1K of flash memory, 32 registers, and only 15 pins, it was decided to write economical and tight code totally in assembly. What the code does, in a nutshell, is to request data from the controller, read it serially, store it in a four 8 bit blocks, read the request pins, and assert the appropriate block of bits on the output port.

Two interrupts are used, a 16 ms watchdog timer and a 2.56 ms timer 0 overflow. The watchdog exists because noise in the system introduced by mouse polling sometimes threw off the state of the data line, forcing the N64 code into an infinite loop. Basically, it end the response early and the code would be stuck listening for a falling edge that does not come. The timer 0 overflow enables the poll request, it samples the N64 fast enough to give a feeling of real time control, while adding enough delay to let the N64 sleep and handle requests from the PS/2 code.

After initialization, the code enters an infite loop. At the beginning of each iteration, it shifts the values from the request pins into a request register. It then outputs the requested bits to PortB. All of this operates on a 5 V logic level.

Whenever polling is enabled, the chip asserts the poll sequence on PinD 0. The difference between the 5v chip operation and the ~3.3V N64 operation combined with the bidirectional floating nature of the data line necessitated some outside hardware. Two diodes in series with a good resistor drop the voltage to around 3.8 V, a little high for the N64, but bearable (and desireable for reasons discussed later.) This voltage is fed into the Vcc port of the controller, causing the data line to float at the same level. The request pin, of course, does not write directly to the data line, instead it writes to the base of a PNP bipolar transistor. If the output of the request pin is the logical inverse of the request sequence, the appropriate request will appear on the data line.

About 2 microseconds after a request, the N64 will start outputting the button states. Since the data voltage is about 3.8 V, the read pin (PinD 1) can be directly triggered without pullin the logic back up to 5V. The code delays 2 microseconds and then keeps checking for a low voltage on the data line. Once the low voltage is encountered, we know that bit0 has been output. We delay another 2 microseconds till we're in the middle of the bit. As it turns out, the voltage level at this point in the bit gives the logical value. The chip then waits for a high voltage, once it knows the line is high, it then waits for the next falling edge and reads the next bit. After every bit is read, the value is shifted into the appropriate register. Once all the bits are read, the loop returns to its origin.

##PS/2 Mouse Emulation

To interface with the PS/2 port, we chose the class's standard Atmel 90s8515 running at 4Mhz. Since the PS/2 interface uses an open collector scheme at 5V, we were able to connect the MCU directly to the data and clock lines coming from the host computer. To send a 1, we let the line float high by setting the appropriate MCU pin to input mode, with its pullup resistor activated (DDRx.y = 0, PORTx.y = 1). To send a 0, we set the pin to output mode, and pulled the line down to 0 (DDRx.y = 1, PORTx.y = 0).

With this electrical scheme and an appropriate program, the MCU emulates a standard PS/2 3-button mouse, responding to the host's queries and commands just as a regular mouse would. At the same time, it polls the N64 input microcontroller on a 10-bit bus (2 command lines, 8 data lines) for information about the Nintendo controller's state, then translating it to PS/2 mouse movement packets. The details of the PS/2 mouse interface were taken almost entirely from Adam Chapweske's fantastic pages at the University of North Dakota. The emulation program is written in a mixed style: AVR assembly for the low-level PS/2 interfacing routines and C for the higher-level routines that poll the Nintendo 64 and emulate the high-level behavior of a mouse.

A standard PS/2 mouse sends movement data to the host computer with 3-byte packets. The first byte contains the state of the mouse's buttons, the second byte contains the change in the mouse's X position, and the third byte contains the change in the mouse's Y position (since the last movement packet was sent). Positive X corresponds to left on the screen and positive Y corresponds to the up direction (for Windows mouse drivers anyway). Again, for all the details on the PS/2 protocol look at Adam's page here. The source code for the PS/2 emulator is in ps2.c.

##PS/2 Host Emulation

During debugging we found it useful to write a small program to emulate the commands a host might send to a mouse. This little device simply sends commands on the PS/2 line depending on which of 8 external buttons is pressed. Using this, we could debug the mouse emulator's host-to-device communications without rebooting our test computer as often(to redo the mouse startup sequence). Its source code can be found in hostutil.c.