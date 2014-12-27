'© Jakob Schaefer 2010
'This source code file is for personal use only. Use at own risk.

$regfile = "m8def.dat"

'8MHz = highest internal frequency of the ATMega8
$crystal = 8000000

'configuring the ADC, its resolution is 10bit per channel
Config Adc = Single , Prescaler = Auto , Reference = Avcc
Start Adc

'pins for the output of the X-axis
Config Portb.1 = Output
Config Portb.2 = Output

'pins for the output of the y-axis
Config Portb.3 = Output
Config Portb.4 = Output

'we want to activate the internal pullup resistors for the I/O pins we don't need to prevent them from floating free
'...many, many unused I/O pins....
Portb.5 = 1
Portc.0 = 1
Portc.3 = 1
Portc.4 = 1
Portc.5 = 1
Portd.0 = 1
Portd.1 = 1
Portd.2 = 1
Portd.3 = 1
Portd.4 = 1
Portb.6 = 1
Portb.7 = 1
Portd.5 = 1
Portd.6 = 1
Portd.7 = 1
Portb.0 = 1



'variables in which the 10bit value of the ADC will be stored
Dim X As Integer , Y As Integer

'R will affect the range of X- and Y-axis (1024 * R / 128)
'R=20 means the range of each axis will be 160 steps (1024*20/128=160)
'If you drive Pinb.5 low (Pin 19), R will be 21 resulting in a slightly higher range of 168 steps
Dim R As Byte
R = 20
If Pinb.5 = 0 Then R = 21

'init X axis. t
X = Getadc(1)
X = X * R
Shift X , Right , 7 , Signed

'init Y-axis
Y = Getadc(2)
Y = Y * R
Shift Y , Right , 7 , Signed

'variables to store the old X and Y values for comparison in the next round
Dim Oldx As Integer , Oldy As Integer
Oldx = X
Oldy = Y


Dim Xwheel As Byte , Ywheel As Byte
Xwheel = &B11001100
Ywheel = &B11001100

Dim I As Byte

'in these two variables we're storing the number of steps we have to process for each axis
Dim Xsteps As Integer , Ysteps As Integer



'main loop:
'-------------
Do

'get and store X-value; resolution = 10 bit * 10 / 64 = 160 steps / axis
X = Getadc(1)
X = X * R
Shift X , Right , 7 , Signed

'get and store X-value; resolution = 10 bit * 10 / 64 = 160 steps / axis
Y = Getadc(2)
Y = Y * R
Shift Y , Right , 7 , Signed

'calculate the number of steps we have to process
Xsteps = X
Xsteps = Xsteps - Oldx
Ysteps = Y
Ysteps = Ysteps - Oldy



'stay in while-loop until all steps of the X- and Y-axis are processed
While Xsteps <> 0 Or Ysteps <> 0

   'steps of the x-axis
   If Xsteps > 0 Then
      Rotate Xwheel , Left , 1
      Xsteps = Xsteps - 1
   Elseif Xsteps < 0 Then
      Rotate Xwheel , Right , 1
      Xsteps = Xsteps + 1
   End If

   'steps of the y-axis
   If Ysteps > 0 Then
      Rotate Ywheel , Right , 1
      Ysteps = Ysteps - 1
   Elseif Ysteps < 0 Then
      Rotate Ywheel , Left , 1
      Ysteps = Ysteps + 1
   End If

   'output the new gray code to the pins for XA, XB, YA & YB
   Portb.1 = Xwheel.1                                       'XA
   Portb.2 = Xwheel.2                                       'XB
   Portb.3 = Ywheel.1                                       'YA
   Portb.4 = Ywheel.2                                       'YB


   'we have to wait a little bit for processing the next steps because otherwise
   'it would be too fast and the IC in the N64 controller would skip some steps
   Waitus 10

Wend

'store the values of both axis for comparison in the next round
Oldx = X
Oldy = Y

Loop
End