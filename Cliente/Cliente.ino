/*  ----------------------------------------------------------------
--------------------------------------------------------------------
                           EMISOR WIFI
                          -------------

Diana de impacto con dos leds (rojo/verde) y buzzer.

CONFIGURACION DE PINES:

ARDUINO NANO


SENSOR => D3
LED ROJO => D4;
LED VERDE => D5;
BUZZER => D6;

IRQ => D2
CE  -> D9
CSN -> D10
MOSI => D11
MISO => D12
SCK => D13

--------------------------------------------------------------------
--------------------------------------------------------------------

TODOS LOS LEDS APAGADOS INDICA DIANA EN MODO ESCUCHA
LED ROJO, MODO SELECCIONADO Y ESPERANDO IMPACTO
LED VERDE, IMPACTO RECIBIDO (DEPENDE DEL MODO TRASMITIRA EL IMPACTO O NO, SE APAGARA TRAS X SEG).
*/
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

//inicializacion de radio

RF24 radio(9, 10); // CE, CSN
const byte address[6] = { 0xABCDABCD71LL };
const byte addressRead[6] = { 0x544d52687CLL };

// d1-dX nombre de cada diana.
//Cada diana debe ser nombrada de forma distinta, si le ponemos el mismo nombre a dos dianas
//no funcionara correctamente.
char text[3] = "d1";
int diana = 1;

// almacenamiento del texto en modo escucha.
// Primer digito modo de juego(m1,m2,...) _ segundo grupo tiempo modo activo envio (m1_3, m1_2...)
// Ejemplo m2_3  (modo2, 3 segundos de activacion)
char textEscucha[32] = ""; 

const int sensorPin = 3;
int sensor = 0;

int modo = 0; //modo escucha 0, modo emisor 1
int modoJuego = 0;
int tiempoJuego = 0;
String opciones;

//ILUMINACION
const int ledRojo = 4;
const int ledVerde = 5;
const int buzzer = 6;

int firstIndex = 0;
int lastIndex = 0;

unsigned long t1Impacto = 0;
unsigned long t2Impacto = 0;

void setup(void)
{
	Serial.begin(9600);
	radio.begin();

	if (radio.isChipConnected()) {
		Serial.println("Chip conectado");
	}
	else {
		Serial.println("Chip no conectado");
	}

	if (radio.isPVariant()) {
		Serial.println("CHIP: nRF24L01 +");
	}
	else {
		Serial.println("CHIP: NO nRF24L01 +");
	}

// potencia de emision establecido en bajo, se puede aumentar segun necesidad
// @TODO hay que realizar test de alcance para ver como va en low
// @TODO 
	radio.setChannel(50);
	radio.setPALevel(RF24_PA_LOW); 
	//radio.setRetries(150, 10);

	radio.openWritingPipe(address);
	radio.openReadingPipe(1, addressRead);
	radio.startListening();

	//SENSOR DE IMPACTO

	pinMode(sensorPin, INPUT);
	digitalWrite(sensorPin, HIGH);  

	//LED ROJO
	pinMode(ledRojo, OUTPUT);
	digitalWrite(ledRojo, LOW);  

	//LEDS VERDE
	pinMode(ledVerde, OUTPUT);
	digitalWrite(ledVerde, LOW);  

	//BUZZER
	pinMode(buzzer, OUTPUT);
	digitalWrite(buzzer, HIGH);  

}

void loop(void)
{	
	switch (modo) {

	// modo escucha
	case 0:
		Serial.println("Modo Escucha.");
		//hay que poner los leds en verde en el modo escucha
		apagaLedRojo();
		apagaLedVerde();
		radio.closeReadingPipe(1);
		radio.openReadingPipe(1, addressRead);
		radio.startListening();
   // si no necesitamos el wifi se puede eliminar el codigo dentro del siguiente IF
		if (radio.available()) {
			radio.read(&textEscucha, sizeof(textEscucha));
			Serial.println("Lectura Realizada");
			String modoEnviado = textEscucha;
			Serial.println("Modo recibido: " + modoEnviado);
			opciones = textEscucha;
			if (modoEnviado.substring(0, 1) == "m") {
				//Serial.println(modoEnviado.substring(1, 2));
				modo = modoEnviado.substring(1, 2).toInt();
				Serial.println(modo);
				radio.closeReadingPipe(1);
			}
			else {
				modo = 0;
			}
		}
		// chequeamos el impacto para ponerlo en diana autonoma si no hay wifi
		sensor = digitalRead(sensorPin);
		if (sensor != 0) {
			t1Impacto = millis();
			Serial.println("impacto recibido");
			//leds en verde(impacto recibido)
			apagaLedRojo();
			enciendeLedVerde();
			delay(900); // si incrementamos este valor el led  que marca el impacto permanecera mas tiempo encendido
      //@ TODO activar sonido buzzer
		}
		delay(100);
		break;

	//modo recorrido
	case 1:
		//leds en rojo (espera de impacto)
		enciendeLedRojo();
		apagaLedVerde();
		radio.stopListening();
		
		Serial.println("Modo de juego: Recorrido");
		sensor = digitalRead(sensorPin);
		if (sensor != 0) {
			for (int i = 0; i < 5; i++)
			{
				radio.stopListening();
				radio.write(text, sizeof text);
				Serial.println("Datos enviados");
				//leds en verde(impacto recibido)
				apagaLedRojo();
				enciendeLedVerde();
       //@ TODO activar sonido buzzer
				delay(1000);
			}

			modo = 0;
		}
		break;

	case 2:
		//m2_10_3
		/*
		Modo enviado: m2_18_2   MODO 2, DIANA 18, TIEMPO ENCENDIDO 2
		*/
		
		Serial.println("Modo Diana Aleatoria");
		firstIndex = opciones.indexOf("_");
		lastIndex = opciones.lastIndexOf("_");

		// si es la diana actual
		
		if (opciones.substring(firstIndex + 1, lastIndex).toInt() == diana) {
			long t1 = millis();
			int time = opciones.substring(lastIndex + 1).toInt();
			enciendeLedRojo();
			//Serial.println(millis() + " " +(t1 + (time * 1000)));
			do
			{
				sensor = digitalRead(sensorPin);
				if (sensor != 0) {
						radio.stopListening();
						radio.openWritingPipe(address);
						radio.write(text, sizeof text);
						Serial.println("Datos enviados");
						//leds en verde(impacto recibido)
						apagaLedRojo();
						enciendeLedVerde();
           //@ TODO activar sonido buzzer
						delay(500);
						apagaLedRojo();
						apagaLedVerde();
						modo = 0;
						break;
				}
			} while (millis() < (t1 + (time * 1000)));
		}
		//apagaLedRojo();
		//apagaLedVerde();
		modo = 0;
		break;

	case 3:
		/*
		Modo enviado: m3_18   MODO 3, DIANA 18
		*/
		Serial.println("Modo Diana Aleatoria");
		firstIndex = opciones.indexOf("_");
		lastIndex = opciones.length();
		// si es la diana actual
		if (opciones.substring(firstIndex + 1, lastIndex).toInt() == diana) {
			enciendeLedRojo();
			do
			{
				sensor = digitalRead(sensorPin);
				if (sensor != 0) {
					radio.stopListening();
					radio.openWritingPipe(address);
					radio.write(text, sizeof text);
					Serial.println("Datos enviados");
					//leds en verde(impacto recibido)
					apagaLedRojo();
					enciendeLedVerde();
         //@ TODO activar sonido buzzer
					delay(500);
					apagaLedRojo();
					apagaLedVerde();
					modo = 0;
					break;
				}
			} while (modo != 0);
		}
		modo = 0;
		break;

	case 4:

		Serial.println("Modo TEST");
		firstIndex = opciones.indexOf("_");
		lastIndex = opciones.lastIndexOf("_");

		enciendeLedRojo();
		apagaLedVerde();
		radio.stopListening();

		Serial.println("Modo de juego: TEST SISTEMA DIANA");
		
		Serial.println(opciones.substring(firstIndex + 1, firstIndex + 3));
		Serial.println(text);
		enciendeLedRojo();
		delay(1000);
		if (opciones.substring(firstIndex + 1, lastIndex) == text) {
			enciendeLedRojo();		
			radio.stopListening();
			radio.openWritingPipe(address);
			radio.write(text, sizeof text);
			Serial.println("Datos enviados");
			//leds en verde(impacto recibido)
			apagaLedRojo();
			enciendeLedVerde();
     //@ TODO activar sonido buzzer
			delay(5000);
			
		}
		modo = 0;
		break;

	default:
		modo = 0;
		break;
	}
}

boolean wifiRead() {
	if (radio.available()) {
		radio.read(&textEscucha, sizeof(textEscucha));
		Serial.println("Lectura Realizada");
		return false;

	}
	return true;
}

void enciendeLedVerde() {
	digitalWrite(ledVerde, LOW);
}

void enciendeLedRojo() {
	digitalWrite(ledRojo, LOW);
}

void apagaLedVerde() {
	digitalWrite(ledVerde, HIGH);
}

void apagaLedRojo() {
	digitalWrite(ledRojo, HIGH);
}

//@ TODO crear metodo sonido buzzer
