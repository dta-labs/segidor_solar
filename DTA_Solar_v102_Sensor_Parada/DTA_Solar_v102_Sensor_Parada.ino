/*****************************************************************************
 *                                                                           *
 *                    DTA-Energy es una marca de DTA-Labs                    *
 *                                                                           *
 *                     SEGUIDOR SOLAR RCT v1.02 Cornelio                     *
 *                                                                           *
 *              DTA-Labs   https://dta.labs.com.mx                           *
 *              DTA-Energy https://dta.labs.com.mx/dta-energy                *
 *              Contacte con uno de nuestros representantes:                 *
 *              correo: dta.las.contact@gmail.com                            *
 *              Teléfono/WhatsApp: 625 152 3176                              *
 *                                                                           *
 *              Conector de salida:                                          *
 *              1. Motor+                                                    *
 *              2. Motor-                                                    *
 *              3. Vcc (+5V)                                                 *
 *              4. Gnd (-)                                                   *
 *              5. Referencia 1 (Input 1)                                    *
 *              6. Referencia 2 (Input 2)                                    *
 *              7. Posición (analógica)                                      *
 *              8. Gnd (-)                                                   *
 *                                                                           *
 *****************************************************************************/
 
#include <Wire.h>
#include <RtcDS3231.h>
#include <LiquidCrystal_I2C.h>
#include "analogo.h"

/************* Configuración del display líquido 16x2 *************/

LiquidCrystal_I2C lcd(0x27, 16, 2);               // Direcciones que puede tener el LCD (0x3f,16,2) | (0x27,16,2) | (0x20,16,2)
char DiaSemana[][4] = {"Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab"};

/************* Configuración del reloj en tiempo real DS1302 *************/

RtcDS3231<TwoWire> Rtc(Wire);

/************* Configuración de los pines *************/

const int pinBtnModo           = 5;               // Botón Modo de trabajo
const int pinBtnMenos          = 6;               // Botón Config -
const int pinBtnMas            = 7;               // Botón Config +
const int pinMotorFF           = 8;               // Motor FF
const int pinMotorRR           = 9;               // Motor RR
const int pintSensorHall       = 10;              // Sensor del pistón
const int pinComunicacionTx    = 11;              // Comunicación Tx
const int pinComunicacionRx    = 12;              // Comunicación Rx
const int sensorAnalogico      = A3;              // Sensor de pistón en el inicio << Sensor analógico >>
const int encendido            = LOW;
const int apagado              = HIGH;

/************* Configuración de variables *************/

int posicionActual          = 0;
int modoTrabajo             = 0;                  // Modos de prabajabo { 0: Operación | 1: Año | 2: Mes | 3: Día | 4: Hora | 5: Minutos | 6: Posición Manual | 7: Posición Inicial | 8: Autocalibrar }
int modoDigitalAnalogico    = 0;                  // Modo de sensado { 0: Digital (Pulsos) | 1: Analógico (Resistencia Lineal) }
int Tpmax                   = 120;                // Tiempo máximo del pistón en segundos (2 min)
int Toperacion = Tpmax * 1000 / 150;              // Tiempo máximo del pistón en milisegundos entre la cantidad de pasosMax (10h)
int direccion               = 0;
int Anno                    = 2021;
int Mes                     = 1;
int Dia                     = 1;
int Hora                    = 0;
int Minutos                 = 0;
int ajuste                  = 3000;
int horaInicio              = 9;
int horaFin                 = 19;
int pasosMax                = 150;
RtcDateTime fechaHora;

void setup() {
  Serial.begin(9600);
  inicializarLCD();
  inicializarRTC();
  inicializarArduino();
  lcd.clear();
}

/************* Inicializaciones *************/

void calcularTiempoOperacion() {
  horaInicio = (Mes >= 4 && Mes <= 9) ? 10 : 9;    // horaInicio 10am -> Abril - Septiembre
  horaFin = (Mes >= 4 && Mes <= 9) ? 20 : 19;      // horaFin 7pm -> Abril - Septiembre
  pasosMax = (horaFin - horaInicio) * 60 / 4;      // pasosMax cantidad de horas por 15 pasos
  int Tp = Tpmax * 1000;                           // Convertir el tiempo máximo del pistón a milisegundos
  Toperacion = Tp / pasosMax;                      // Tiempo del pistón entre los pasosMax a moverse
}

void inicializarArduino() {
  pinMode (pinMotorFF, OUTPUT);
  pinMode (pinMotorRR, OUTPUT);
  pinMode (pinBtnMas, INPUT);
  pinMode (pinBtnMenos, INPUT);
  pinMode (pinBtnModo, INPUT);
  pinMode (pintSensorHall, INPUT);
  pinMode (pinComunicacionTx, OUTPUT);
  pinMode (pinComunicacionRx, INPUT);
  digitalWrite (pinMotorFF, HIGH);
  digitalWrite (pinMotorRR, HIGH);
  calcularTiempoOperacion();
  moveToLimit(-1);
  moveToLimit(-1);
  posicionActual = 0;
  Serial.println("inicializarArduino OK");
}

void loop() {
  getFechaHora();
  cambiarModo();
  switch (modoTrabajo) {
    case 1:
      // Año
      modificarAnno();
      break;
    case 2:
      // Mes
      modificarMes();
      break;
    case 3:
      // Día
      modificarDia();
      break;
    case 4:
      // Hora
      modificarHora();
      break;
    case 5:
      // Minutos
      modificarMinutos();
      break;
    case 6:
      // Modo de sensado
      digitalAnalogico();
      break;
    case 7:
      // Ajustar Tiempo
      ajustarTiempo();
      break;
    case 8:
      // Ajustar Hora Inicio
      ajustarHoraInicio();
      break;
    case 9:
      // Autocalibrar
      autocalibrar();
      break;
    case 10:
      // Posición Manual
      posicionManual();
      break;
    case 11:
      // Posición Inicial
      visualizarCadenasLCD("Calibrar sistema", "Piston a inicio ", -1);
      if (botonAccionado(pinBtnMas)) {
        moveToLimit(-1);
        moveToLimit(-1);
        posicionActual = 0;
      }
      break;
    default:
      visualizarCadenasLCD("DTA-Labs Energia", printDateTime(fechaHora), -1);
      calculaPosicion();
      delay(5000);
      break;
  }
}

void getFechaHora() {
    if(fechaHora.Hour() != Rtc.GetDateTime().Hour() && fechaHora.Minute() != Rtc.GetDateTime().Minute()) {
      Serial.print("Fecha/Hora: ");
      Serial.print(Dia);
      Serial.print("/");
      Serial.print(Mes);
      Serial.print("/");
      Serial.print(Anno);
      Serial.print(" ");
      Serial.print(Hora);
      Serial.print(":");
      Serial.println(Minutos);
    }
    fechaHora = (Rtc.GetDateTime() < fechaHora) ? fechaHora : Rtc.GetDateTime();
    fechaHora = Rtc.GetDateTime();
    Anno      = fechaHora.Year();
    Mes       = fechaHora.Month();
    Dia       = fechaHora.Day();
    Hora      = fechaHora.Hour();
    Minutos   = fechaHora.Minute();
}

bool botonAccionado(int btn) {
  bool result = false;
  if(digitalRead(btn) == 0) {
    result = true;
    while (digitalRead(btn) == 0) {
      delay(1);
    }
    lcd.clear();
  }
  return result;
}

void cambiarModo() {
  if (botonAccionado(pinBtnModo)) {
    Serial.println("Cambiar modo...");
    modoTrabajo = (modoTrabajo + 1) > 11 ? 0 : modoTrabajo + 1;
  }  
}

void modificarAnno() {
  visualizarCadenasLCD("Calibrar sistema", "Ano: ", Anno);
  Anno = (fechaHora.Year() < Anno) ? Anno : fechaHora.Year();
  if (botonAccionado(pinBtnMas)) {
    Anno++;
    actualizarFechaHora();
  }
  if (botonAccionado(pinBtnMenos)) {
    Anno = (Anno - 1 <= 2021) ? 2021 : Anno - 1;
    actualizarFechaHora();
  }
}

void modificarMes() {
  visualizarCadenasLCD("Calibrar sistema", "Mes: ", Mes);
  Mes = (fechaHora.Month() < Mes) ? Mes : fechaHora.Month();
  if (botonAccionado(pinBtnMas)) {
    Mes = (Mes + 1) > 12 ? 1 : Mes + 1;
    actualizarFechaHora();
  }
  if (botonAccionado(pinBtnMenos)) {
    Mes = (Mes - 1) < 1 ? 12 : Mes - 1;
    actualizarFechaHora();
  }
}

void modificarDia() {
  visualizarCadenasLCD("Calibrar sistema", "Dia: ", Dia);
  Dia = (fechaHora.Day() < Dia) ? Dia : fechaHora.Day();
  if (botonAccionado(pinBtnMas)) {
    Dia = (Dia + 1) > 31 ? 1 : Dia + 1;
    actualizarFechaHora();
  }
  if (botonAccionado(pinBtnMenos)) {
    Dia = (Dia - 1) < 1 ? 31 : Dia - 1;
    actualizarFechaHora();
  }
}

void modificarHora() {
  visualizarCadenasLCD("Calibrar sistema", "Hora: ", Hora);
  Hora = (fechaHora.Hour() < Hora) ? Hora : fechaHora.Hour();
  if (botonAccionado(pinBtnMas)) {
    Hora = (Hora + 1) > 23 ? 0 : Hora + 1;
    actualizarFechaHora();
  }
  if (botonAccionado(pinBtnMenos)) {
    Hora = (Hora - 1) < 0 ? 23 : Hora - 1;
    actualizarFechaHora();
  }
}

void modificarMinutos() {
  visualizarCadenasLCD("Calibrar sistema", "Minutos: ", Minutos);
  Minutos = (fechaHora.Minute() < Minutos) ? Minutos : fechaHora.Minute();
  if (botonAccionado(pinBtnMas)) {
    Minutos = (Minutos + 1) > 59 ? 0 : Minutos + 1;
    actualizarFechaHora();
  }
  if (botonAccionado(pinBtnMenos)) {
    Minutos = (Minutos - 1) < 0 ? 59 : Minutos - 1;
    actualizarFechaHora();
  }
}

void posicionManual() {
  visualizarCadenasLCD("Mover piston    ", "Manual          ", -1);
  if (botonAccionado(pinBtnMas)) {
    moverPiston(1, "Mover piston    ");
    delay(Toperacion + ajuste);
    moverPiston(0, "Manual          ");
  }
  if (botonAccionado(pinBtnMenos)) {
  //    moverPiston(-1, "Mover piston    ");
  //    delay(Toperacion + ajuste);
  //    moverPiston(0, "Manual          ");

    for (int i = 0; i < (Toperacion + ajuste) / 10; i++) {
      moverPiston(-1, "Mover piston    ");
      delay(10);
    }
    moverPiston(0, "Manual          ");

  }
}

void digitalAnalogico() {
  visualizarCadenasLCD("Calibrar sistema", modoDigitalAnalogico == 0 ? "Modo: Digital   " : "Modo: Analogico ", -1);
  if (botonAccionado(pinBtnMas)) {
    modoDigitalAnalogico = 0;
  }
  if (botonAccionado(pinBtnMenos)) {
    modoDigitalAnalogico = 1;
  }
}

void ajustarTiempo() {
  visualizarCadenasLCD("Calibrar sistema", "Tiempo (s): ", Tpmax);
  if (botonAccionado(pinBtnMas)) {
    Tpmax += 60;
    calcularTiempoOperacion();
  }
  if (botonAccionado(pinBtnMenos)) {
    Tpmax = (Tpmax - 1) <= 60 ? 60 : Tpmax - 60;
    calcularTiempoOperacion();
  }
}

void ajustarHoraInicio() {
  visualizarCadenasLCD("Calibrar sistema", "Hora inicio: ", horaInicio);
  if (botonAccionado(pinBtnMas)) {
    horaInicio = (horaInicio + 1) > 10 ? 10 : horaInicio + 1;
    calcularTiempoOperacion();
  }
  if (botonAccionado(pinBtnMenos)) {
    horaInicio = (horaInicio - 1) <= 7 ? 7 : horaInicio - 1;
    calcularTiempoOperacion();
  }
}

void autocalibrar() {
  visualizarCadenasLCD("Calibrar sistema", "Ajuste: ", ajuste);
  if (botonAccionado(pinBtnMas)) {
    ajuste = ajuste + 500;
  }
  if (botonAccionado(pinBtnMenos)) {
    ajuste = (ajuste - 500) <= 500 ? 500 : ajuste - 500;
  }
}

void calculaPosicion() {
  Serial.print(".");
  if (horaInicio <= Hora && Hora < horaFin) {
    // Las 8h se toman como la hora cero de las 10h. Se convierte en minutos y se difive entre 4
    int posicionDeseada = (((Hora - horaInicio) * 60) + Minutos) / 4;
    posicionDeseada = (posicionDeseada <= pasosMax) ? posicionDeseada : pasosMax;
    if (posicionActual < posicionDeseada) {
    //      posicionActual++;
      Serial.print("Posición: ");
      Serial.print(posicionActual);
      Serial.print(" / ");
      Serial.print(posicionDeseada);
      Serial.print(" / ");
      Serial.println(pasosMax);
      moveTo(Toperacion); 
      int angulo = posicionActual + 58;
      visualizarCadenasLCD("DTA-Labs Energia", "Angulo: ", angulo);
    }
  } else if (horaFin <= Hora && posicionActual != 0) {
    Serial.println("Regresar a inicio");
    moveToLimit(-1);
    moveToLimit(-1);
    posicionActual = 0;
  }
}

float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void moveToLimit(int dir){
  int pulsos = 0;
  unsigned long tiempo;
  moverPiston(dir, "Mover piston    ");
  if (modoDigitalAnalogico == 0) {
    int temporizador = 0;
    bool repetir = true;
    do {
      int ret = digitalRead(pintSensorHall);
      while (analogRead(sensorAnalogico) < 200 && ret == digitalRead(pintSensorHall) && temporizador < 5000) { // 1250
        delay(100);
        temporizador++;
        Serial.print(".");
      }
      if (analogRead(sensorAnalogico) < 200 && temporizador < 5000 && temporizador > 0) {
  //        temporizador = 0;
  //        ret = digitalRead(pintSensorHall);
  //        while (ret == digitalRead(pintSensorHall) && temporizador < 5000) { 
  //          delay(100);
  //          temporizador++;
  //          Serial.print(".");
  //        }
        delay(100);
        pulsos++;
      } else {
        repetir = false;
      }
    } while (analogRead(sensorAnalogico) < 200 && repetir);
  } else {
    int prevReading=0;
    int currReading=0;
    do {
      prevReading = currReading;
      delay(200);                                 //keep moving until analog reading remains the same for 200ms
      currReading = analogRead(sensorAnalogico);
    } while (prevReading != currReading);
  }
  moverPiston(0, printDateTime(fechaHora));
  Serial.println();
  Serial.print(printDateTime(fechaHora));
  Serial.println(pulsos);
}

void moveTo(int tiempo) {
  tiempo = tiempo + ajuste;
  moverPiston(1, "DTA-Labs Energia");
  delay(tiempo);
  moverPiston(0, "DTA-Labs Energia");
  Serial.println("Motor detenido");
  delay(tiempo + tiempo);
}

void moverPiston(int dir, String txt) {
  direccion = dir;
  switch(direccion){
    case 1:       //extension
      posicionActual = (posicionActual + 1 < 150) ? posicionActual + 1 : 150;
      visualizarCadenasLCD(txt, "Piston >>>      ", posicionActual);
      digitalWrite(pinMotorFF, encendido);
      digitalWrite(pinMotorRR, apagado);
      break;
    case 0:       //stopping
      visualizarCadenasLCD(txt, printDateTime(fechaHora), -1);
      digitalWrite(pinMotorFF, apagado);
      digitalWrite(pinMotorRR, apagado);
      break;
    case -1:      //retraction
      posicionActual = (posicionActual - 1 > 0) ? posicionActual - 1 : 0;
      if (modoDigitalAnalogico == 0) {
  //        visualizarCadenasLCD(txt, "Piston <<<      ", posicionActual);
  //        digitalWrite(pinMotorFF, apagado);
  //        digitalWrite(pinMotorRR, encendido);

        if (analogRead(sensorAnalogico) < 200) {
          visualizarCadenasLCD(txt, "Piston <<<      ", posicionActual);
          digitalWrite(pinMotorFF, apagado);
          digitalWrite(pinMotorRR, encendido);
        }
      } else {
        if(analogRead(sensorAnalogico) > 20) {
          visualizarCadenasLCD(txt, "Piston <<<      ", -1);
          digitalWrite(pinMotorFF, apagado);
          digitalWrite(pinMotorRR, encendido);
        }
      }
      break;
  }
}

///////////////// LCD y RTC /////////////////

void inicializarLCD() {
  Wire.begin();
  lcd.init();
  lcd.backlight();
  visualizarCadenasLCD("DTA-Labs Energia", "\"SEGUIDOR SOLAR\"", -1);
  delay(5000);
  Serial.println("inicializarLCD OK");
}

void visualizarCadenasLCD(String cad1, String cad2, int num) {
  lcd.setCursor(0,0);
  lcd.print("                ");
  lcd.setCursor(0,0);
  lcd.print(cad1);
  lcd.setCursor(0,1);
  lcd.print("                ");
  lcd.setCursor(0,1);
  lcd.print(cad2);
  if (num != -1) {
    lcd.print(num, DEC);
  }
}

void inicializarRTC() {
  Serial.print("Compilación: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);
  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();
  if (!Rtc.IsDateTimeValid()) {
    if (Rtc.LastError() != 0) {
        Serial.print("Error de comunicación con el RTC = ");
        Serial.println(Rtc.LastError());
    } else {
      // Causas comunes:
      // 1) La primera vez que ejecutó, el dispositivo aún no se estaba ejecutando
      // 2) Batería baja o ausente
      Serial.println("Error: RTC perdió confianza en la Fecha y Hora!");
      Rtc.SetDateTime(compiled);
    }
  }
  if (!Rtc.GetIsRunning()) {
    Serial.println("Error: RTC no está corriendo, inicialice ahora!");
    Rtc.SetIsRunning(true);
  }
  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) {
    Serial.println("Atención: RTC más antiguo que el tiempo de compilación! (Actualice Fecha y Hora)");
    Rtc.SetDateTime(compiled);
  } else if (now > compiled)     {
    Serial.println("Atención: RTC más actualizado que el tiempo de compilación. (es lo esperado)");
  } else if (now == compiled)     {
    Serial.println("Atención: RTC igual al tiempo de compilación! (No esperado pero está bien)");
  }
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 
  Anno = now.Year() > 2021 ? now.Year() : 2021;
  Mes = now.Month() > 1 ? now.Month() : 1;
  Dia = now.Day() > 1 ? now.Day() : 1;
  Hora = now.Hour() > 0 ? now.Hour() : 0;
  Minutos = now.Minute() > 1 ? now.Minute() : 1;
  Serial.print("inicializarRTC ");
  mostrarFechaHora();
  Serial.println();
}

void mostrarFechaHora() {
  Serial.print(Dia);
  Serial.print("/");
  Serial.print(Mes);
  Serial.print("/");
  Serial.print(Anno);
  Serial.print("/");
  Serial.print(" ");
  Serial.print(Hora);
  Serial.print(":");
  Serial.print(Minutos);
}

#define countof(a) (sizeof(a) / sizeof(a[0]))

String printDateTime(const RtcDateTime& dt) {
  char datestring[20];
  snprintf_P(datestring, 
          countof(datestring),
          PSTR("%02u/%02u/%04u %02u:%02u"),
          dt.Day(),
          dt.Month(),
          dt.Year(),
          dt.Hour(),
          dt.Minute());
  return datestring;
}

void actualizarFechaHora() {
  char meses[][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  Serial.println("Actualizabdo Fecha y Hora...");
  char datestring[20];
  snprintf_P(datestring, countof(datestring), PSTR("%s %02u %04u"), meses[Mes - 1], Dia, Anno);
  char timestring[20];
  snprintf_P(timestring, countof(timestring), PSTR("%02u:%02u:%02u"), Hora, Minutos, 0);
  fechaHora = RtcDateTime(datestring, timestring);
  Rtc.SetDateTime(fechaHora);
}
