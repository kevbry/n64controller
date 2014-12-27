'© Jakob Schaefer 2010
'This source code file is for personal use only. Use at own risk.

$regfile = "attiny24.dat"

'8MHz = highest internal frequency of the ATtiny24
$crystal = 8000000

'configuring the ADC, its resolution is 10bit per channel
Config Adc = Single , Prescaler = Auto
Start Adc

'pins for the output of the X-axis
Config Porta.7 = Output
Config Portb.2 = Output

'pins for the output of the Y-axis
Config Porta.4 = Output
Config Porta.3 = Output

'we want to activate the internal pullup resistors for the I/O pins we don't need to prevent them from floating free
Portb.0 = 1
Portb.1 = 1
Porta.0 = 1
Porta.1 = 1
Porta.2 = 1


'variables in which the 10bit value of the ADC will be stored
Dim X As Integer , Y As Integer

'R will affect the range of X- and Y-axis (1024 * R / 128)
'R=20 means the range of each axis will be 160 steps (1024*20/128=160)
'If you drive PinA.0 low (Pin 13), R will be 21 resulting in a slightly higher range of 168 steps
Dim R As Byte
R = 20
If Pina.0 = 0 Then R = 21

'init X-axis
X = Getadc(6)
X = Getadc(6)
X = X * R
Shift X , Right , 7 , Signed

'init Y-axis
Y = Getadc(5)
Y = Getadc(5)
Y = Y * R
Shift Y , Right , 7 , Signed

'variables to store the old X and Y values for comparison in the next round
Dim Oldx As Integer , Oldy As Integer
Oldx = X
Oldy = Y


'much like the optical wheels in the real N64 controller we create xwheel and ywheel.
'imagine those 2 bytes as wheels; we're fixating on the 1st and 2nd bit of those wheels.
'to go one step down or up all we have to do is to rotate the byte left or right
'
' 1 1          0 1         0 0         1 0         1 1
'0   0 <-XA   0   1<-XA   1   1<-XA   1   0<-XA   0   0<-XA
'0   0 <-XB   1   0<-XB   1   1<-XB   0   1<-XB   0   0<-XB
' 1 1          1 0         0 0         0 1         1 1
'
'00->10->11->01->00 those are the gray codes for going down and we get these new
'gray codes simply by rotating the byte :)
Dim Xwheel As Byte , Ywheel As Byte
Xwheel = &B11001100
Ywheel = &B11001100

'in these two variables we're storing the number of steps we have to process for each axis
Dim Xsteps As Integer , Ysteps As Integer



'main loop:
'-------------
Do

'get and store X-value
X = Getadc(6)
X = X * R
Shift X , Right , 7 , Signed

'get and store X-value
Y = Getadc(5)
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

   'output the new gray code to the pins for XA and XB...
   Porta.7 = Xwheel.1
   Portb.2 = Xwheel.2
   '...and YA & YB
   Porta.4 = Ywheel.1
   Porta.3 = Ywheel.2


   'we have to wait a little bit for processing the next steps because otherwise
   'it would be too fast and the IC in the N64 controller would skip some steps
   Waitus 10

Wend

'store the values of both axis for comparison in the next round
Oldx = X
Oldy = Y

Loop
End