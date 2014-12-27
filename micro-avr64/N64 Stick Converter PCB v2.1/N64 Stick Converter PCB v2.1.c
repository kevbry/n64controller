/*
 * N64_Stick_Converter_PCB_v2.1.c
 *
 * Created: 30.06.2013 10:31:30
 * Author: Jakob Schäfer
 *
 * ONLY FOR YOUR OWN PERSONAL USE! COMMERCIAL USE PROHIBITED!
 * NUR FÜR DEN EIGENGEBRAUCH! GEWERBLICHE NUTZUNG VERBOTEN! 
 *
 * fusebyte low:	0x42
 * fusebyte high:	0xDF
 *
 * --------------------------------------------------------------
 * ATtiny24A pin	|	function			 					|
 * (PDIP / SOIC)	|											|
 * -------------------------------------------------------------|
 * 1				|	VCC = 									|
 * 					|	N64 controller PCB pin no. 5.			|
 * 					|	Bypass to GND with 100 nF capacitor		|
 * -------------------------------------------------------------|
 * 2				|	N64 controller PCB pin no. 6			|
 * -------------------------------------------------------------|
 * 3				|	N64 controller PCB pin no. 3			|
 * -------------------------------------------------------------|
 * 4				|	RESET for programming					|
 * 					|	Connect with 10 kOhm resistor to VCC	|
 * -------------------------------------------------------------|
 * 5				|	Enable normal calibration (active low)	|
 * -------------------------------------------------------------|
 * 6				|	N64 controller PCB pin no. 2			|
 * -------------------------------------------------------------|
 * 7				|	N64 controller PCB pin no. 1			|
 * 					|	MOSI for programming					|
 * -------------------------------------------------------------|
 * 8				|	Enable calibration with dead zones		|
 *					|	(active low)							|
 *					|	MISO for programming					|
 * -------------------------------------------------------------|
 * 9				|	SCK for programming						|
 * -------------------------------------------------------------|
 * 10				|	Invert Y axis (active low)				|
 * -------------------------------------------------------------|
 * 11				|	Invert X axis (active low)				|
 * -------------------------------------------------------------|
 * 12				|	X axis of the stick potentiometer		|
 * -------------------------------------------------------------|
 * 13				|	Y axis of the stick potentiometer		|
 * -------------------------------------------------------------|
 * 14				|	GND =									|
 * 					|	N64 controller PCB pin no. 4			|
 * -------------------------------------------------------------|
 *
 * This is the very same program that's running on these PCB's:
 * http://nfggames.com/forum2/index.php?topic=5023.0
 * http://www.mediafire.com/?encle2go54et96l
 *
 * Yes, german comments only. Deal with it ;)
 *
 * If you want to increase/decrease the range of the stick, then try
 * out new values for the MIN_RANGE and MAX_RANGE constants below:
 */ 



#define F_CPU 1000000UL
#define MIN_RANGE 81							// garantierte +/- Mindestrange pro Achse
#define MAX_RANGE 84							// oberer Grenzwert
#define INVX ( !(PINA&(1<<PINA2)) )
#define INVY ( !(PINA&(1<<PINA3)) )
#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>

uint8_t EEMEM calibrationStep = 5;				// 0 für Standardbetrieb, 1-5 für Kalibrierung
uint16_t EEMEM leftToNeutral;					// für Berechnung der Deadzone und abs. Position
uint16_t EEMEM upToNeutral;					
uint16_t EEMEM xAbsolute;						// absolute neutrale Position bei Verwendung von Deadzones
uint16_t EEMEM yAbsolute;					
uint8_t EEMEM dx = 0;							// Deadzone
uint8_t EEMEM dy = 0;
uint8_t EEMEM cx = 0;							// Verstärkungsfaktoren für Skalierung der ADC Werte
uint8_t EEMEM cy = 0;			

uint16_t GetX(void);							// gibt ADC-Wert für X-Achse zurück (0 bis 1023)
uint16_t GetY(void);							// gibt ADC-Wert für Y-Achse zurück (0 bis 1023)
uint8_t ScaleDown(uint16_t raw16, uint8_t c);	// skaliert auf die passende Range
uint8_t RotateLeft(uint8_t cData);				// rotiert ein Byte um 1 Stelle nach links
uint8_t RotateRight(uint8_t cData);				// rotiert ein Byte um 1 Stelle nach rechts
void Calibration(void);							// umfangreiche Kalibrierungsfunktion
 
 
int main(void)
{	
	int16_t xSteps, ySteps;
	uint16_t x, y, xOld, yOld;
	uint8_t xNeutral8, yNeutral8;
	uint8_t xWheel = 0b11001100;
	uint8_t yWheel = 0b00110011;
	uint16_t xNeutral16, yNeutral16;
	uint8_t xFaktor = eeprom_read_byte(&cx);	
	uint8_t yFaktor = eeprom_read_byte(&cy);
	uint8_t xDeadzone = eeprom_read_byte(&dx);
	uint8_t yDeadzone = eeprom_read_byte(&dy);
	uint8_t useDeadzone;
	
	// Setup Ports
	DDRA = (1<<DDA6)|(1<<DDA7);
	DDRB = (1<<DDB0)|(1<<DDB1);
	PORTA = (1<<PORTA2)|(1<<PORTA3)|(1<<PORTA5);
	PORTB = (1<<PORTB2);
	
	// nicht benötigte Module deaktivieren
	PRR |= (1<<PRTIM0)|(1<<PRTIM1)|(1<<PRUSI);
	
	// 0.25 s warten bis Spannung stabil
	_delay_ms(250);		
	
	// bestimmen, ob Deadzones verwendet werden:
	if ( (xDeadzone==0) && (yDeadzone==0) )
		useDeadzone = 0;
	else
		useDeadzone = 1;
		
	// Setup ADC
	DIDR0 = (1<<ADC0D)|(1<<ADC1D);			// Digital input disable für PA0+1	
	ADMUX = 0x01;							// Kanal 1
	ADCSRA = (1<<ADPS0)|(1<<ADPS1);			// Taktteiler = 8 ==> f_ADC = 125 kHz
	ADCSRA |= (1<<ADEN);					// ADC aktivieren

		
	// 1. ADC Wandlung
	xNeutral16 = GetX();	
	
	// X Wert einlesen:
	xNeutral16 = GetX();	
	// falls Deadzone vorhanden, Absolutwert aus EEPROM lesen
	if (useDeadzone)		
		xNeutral16 = eeprom_read_word(&xAbsolute);				
	xNeutral8 = ScaleDown(xNeutral16, xFaktor);
	xOld = xNeutral8;
	
	// Y Wert einlesen:
	yNeutral16 = GetY();
	// falls Deadzone vorhanden, Absolutwert aus EEPROM lesen
	if (useDeadzone)		
		yNeutral16 = eeprom_read_word(&yAbsolute);							
	yNeutral8 = ScaleDown(yNeutral16, yFaktor);
	yOld = yNeutral8;	
	
	// Wenn PINB.2 = 0, dann beim nächsten Einschalten
	// normalen Calibration Mode aktivieren:
	if( ((1<<PINB2)&PINB)==0){
		eeprom_write_byte(&calibrationStep, 5);		
		eeprom_write_byte(&dx, 0);
		eeprom_write_byte(&dy, 0);		
		while(1);
	}
	
	// Wenn PINA.5 = 0, dann beim nächsten Einschalten
	// Calibration Mode mit Deadzone aktivieren:
	if( ((1<<PINA5)&PINA)==0){
		eeprom_write_byte(&calibrationStep, 1);
		while(1);
	}
	
	// Wenn calibration_step != 0, dann Calibration Mode ausführen
	if ( (eeprom_read_byte(&calibrationStep)) > 0x00 ){
		Calibration();
	}
	
	
    while(1)
    {	
		
		// X Wert einlesen:
		x = GetX();		
		// Deadzone berücksichtigen
		if (useDeadzone){			
			if (x > xNeutral16){
				if (x <= (xNeutral16+xDeadzone))
					x = xNeutral16;
				else
					x = x - xDeadzone;
			}
			if (x < xNeutral16){
				if (x >= (xNeutral16-xDeadzone))
					x = xNeutral16;
				else
					x = x + xDeadzone;
			}
		}	
		// runterskalieren	
		x = ScaleDown( x, xFaktor);
		// auf +/-84 begrenzen
		if (x>xNeutral8)
			if ( (x-xNeutral8) > MAX_RANGE)
				x = xNeutral8 + MAX_RANGE;
		if (x<xNeutral8)
			if ( (xNeutral8-x) > MAX_RANGE)
				x = xNeutral8 - MAX_RANGE;		
						
		// Y Wert einlesen:
		y = GetY();		
		// Deadzone berücksichtigen
		if (useDeadzone){			
			if (y > yNeutral16){
				if (y <= (yNeutral16+yDeadzone))
					y = yNeutral16;
				else
					y = y - yDeadzone;
			}
			if (y < yNeutral16){
				if (y >= (yNeutral16-yDeadzone))
					y = yNeutral16;
				else
					y = y + yDeadzone;
			}
		}		
		// runterskalieren	
		y = ScaleDown(y, yFaktor);		
		// auf +/-84 begrenzen
		if (y>yNeutral8)
			if ( (y-yNeutral8) > MAX_RANGE)
				y = yNeutral8 + MAX_RANGE;
		if (y<yNeutral8)
			if ( (yNeutral8-y) > MAX_RANGE)
				y = yNeutral8 - MAX_RANGE;
	
		//Anzahl Steps berechnen
		xSteps =  (int16_t) x - xOld;
		ySteps =  (int16_t) y - yOld;
		
		// wenn entspr. Jumper gebrückt, invertieren:
		if (INVX)
			xSteps = -xSteps;
		if (INVY)
			ySteps = -ySteps;
		
		//alte Werte für nächsten Durchlauf speichern
		xOld = x;
		yOld = y;
		
		//solange es noch Steps abzuarbeiten gibt:
		while ( (xSteps!=0) || (ySteps!=0) ){
								
			if (xSteps<0){
				xWheel = RotateLeft(xWheel);
				xSteps++;				
			}						
			if (xSteps>0){
				xWheel = RotateRight(xWheel);
				xSteps--;			
			}	
				
			if (ySteps>0){
				yWheel = RotateRight(yWheel);
				ySteps--;				
			}			
			if (ySteps<0){
				yWheel = RotateLeft(yWheel);
				ySteps++;			
			}		
			
			// neue XA/XB- und YA/YB-Werte ausgeben:			
			PORTB = (PORTB&0b11111100)|(xWheel & 0b00000011); 
			PORTA = (PORTA&0b00111111)|(yWheel & 0b11000000);
		}	
		
    }	
	
}


uint16_t GetX(void){	
	ADMUX = 0x01;					// ADC Channel 1	
	ADCSRA |= (1<<ADSC);			// ADC Wandlung starten
	while (ADCSRA & (1<<ADSC));		// warten bis fertig
	return ADC;
}

uint16_t GetY(void){	
	ADMUX = 0x00;					// ADC Channel 0
	ADCSRA |= (1<<ADSC);			// ADC Wandlung starten
	while (ADCSRA & (1<<ADSC));		// warten bis fertig
	return ADC;
}

uint8_t RotateLeft (uint8_t cData){
	uint8_t result;
	if ( cData & (1<<7) )
		result = (cData<<1)|(1<<0);
	else 
		result = (cData<<1);	
	return result;
}

uint8_t RotateRight (uint8_t cData){
	uint8_t result;
	if ( cData & (1<<0) )
		result = (cData>>1)|(1<<7);
	else
		result = (cData>>1);	
	return result;
}

void Calibration(void){	
	
	uint16_t temp1, temp2;
	uint16_t xNeutral16, yNeutral16;
	uint16_t xMin, xMax, yMin, yMax;
	uint16_t timerCounter = 0;
	uint16_t xDeadzone, yDeadzone;
	uint16_t xFaktor, yFaktor;
	uint8_t nSchreibzugriffe = 0;

	
	switch ( eeprom_read_byte(&calibrationStep) )
	{
		
		case 1:
			//Bestimmung left2neutral
			eeprom_write_word(&leftToNeutral, GetX() );			
			//nächster Schritt		
			eeprom_write_byte(&calibrationStep, 2);
			break;
			
		case 2:
			//Bestimmung xabsolute und d_x
			temp1 = GetX();												// temp1 = right2neutral
			temp2 = (eeprom_read_word(&leftToNeutral) + temp1 ) / 2;	// temp2 = xabsolute
			eeprom_write_word(&xAbsolute, temp2);	
			//X Deadzone bestimmen	
			if (temp1>temp2)
				xDeadzone = temp1 - temp2;
			else
				xDeadzone = temp2 - temp1;
			// Wenn Deadzone vorhanden, dann um 1 erhöhen
			if (yDeadzone > 0)
				yDeadzone++;
						
			eeprom_write_byte( &dx, (uint8_t) xDeadzone);				
			//nächster Schritt			
			eeprom_write_byte(&calibrationStep, 3);
			break;
			
		case 3:
			//Bestimmung up2neutral
			eeprom_write_word(&upToNeutral, GetY() );			
			//nächster Schritt		
			eeprom_write_byte(&calibrationStep, 4);
			break;
			
		case 4:
			//Bestimmung yabsolute und d_y
			temp1 = GetY();											// temp1 = down2neutral
			temp2 = (eeprom_read_word(&upToNeutral) + temp1 ) / 2;	// temp2 = yabsolut
			eeprom_write_word(&yAbsolute, temp2);			
			// Y Deadzone bestimmen
			if (temp1>temp2)
				yDeadzone = temp1 - temp2;
			else
				yDeadzone = temp2 - temp1;
			// Wenn Deadzone vorhanden, dann um 1 erhöhen
			if (yDeadzone > 0)
				yDeadzone++;
			eeprom_write_byte( &dy, (uint8_t) yDeadzone);				
			//nächster Schritt			
			eeprom_write_byte(&calibrationStep, 5);
			
		case 5:	
					
			//Bestimmung der Faktoren; Deadzone berücksichtigen!
			xDeadzone = (uint16_t) eeprom_read_byte(&dx);
			yDeadzone = (uint16_t) eeprom_read_byte(&dy);	
						
			// wenn beide Deadzones = 0, dann neutrale 
			// Position einfach aus ADC einlesen
			if ( (xDeadzone==0) && (yDeadzone==0) )					
			{
				xNeutral16 = GetX();		
				yNeutral16 = GetY();
			}
			// ansonsten neutrale Position aus EEPROM lesen
			else
			{
				xNeutral16 = eeprom_read_word(&xAbsolute);
				yNeutral16 = eeprom_read_word(&yAbsolute);
			}
			
			// alle min und max Werte zurücksetzen
			xMin = xNeutral16;								
			xMax = xNeutral16;
			yMin = yNeutral16;
			yMax = yNeutral16;
			
			while (1)
			{
				
				//min und max Werte für X-Achse bestimmen
				temp1 = GetX();				
				if (temp1 > xMax)
					xMax = temp1;					
				if (temp1 < xMin)
					xMin = temp1;
				
				//min und max Werte für Y-Achse bestimmen	
				temp1 = GetY();				
				if (temp1 > yMax)
					yMax = temp1;					
				if (temp1 < yMin)
					yMin = temp1;	
					
				timerCounter++;
				
				// ca. jede Sekunde, aber insg. höchstens 60 Mal:
				if ( (timerCounter>4000) && (nSchreibzugriffe<60) )
				{	
					// Kalibrierung beendet
					eeprom_write_byte(&calibrationStep, 0x00);	
					nSchreibzugriffe++;	
					timerCounter = 0;	
					
					// Faktor für X-Achse:				
					if ( (xMax - xNeutral16) < (xNeutral16 - xMin) )
						temp1 = xMax - xNeutral16;
					else
						temp1 = xNeutral16 - xMin;												
					// Deadzone abziehen													
					temp1 = temp1 - xDeadzone;		
					// Verstärkungsfaktor berechnen				
					xFaktor = ((MIN_RANGE*256)/temp1);							
					// falls Rest übrig, noch einen drauf!	
					if ( ((MIN_RANGE*256)%temp1) > 0  )				
						xFaktor++;			
					// im EEPROM speichern					
					eeprom_write_byte(&cx, (uint8_t) xFaktor);	
					
					// Faktor für Y-Achse:
					if ( (yMax - yNeutral16) < (yNeutral16 - yMin) )
						temp1 = yMax - yNeutral16;
					else
						temp1 = yNeutral16 - yMin;			
					// Deadzone abziehen														
					temp1 = temp1 - yDeadzone;		
					// Faktor berechnen					
					yFaktor = ((MIN_RANGE*256)/temp1);			
					// falls Rest übrig , noch einen drauf!			
					if ( ((MIN_RANGE*256)%temp1) > 0  )
						yFaktor++;
					// im EEPROM speichern
					eeprom_write_byte(&cy, (uint8_t) yFaktor);
				}
			}
	}	
	while (1);					
}

uint8_t ScaleDown(uint16_t raw16, uint8_t c){
	return  (uint8_t) ( (raw16*c) >> 8);	
}