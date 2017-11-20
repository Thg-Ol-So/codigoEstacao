
#include <SoftwareSerial.h>
#include <Nextion.h>

#include "Thread.h"
#include "ThreadController.h"
 // importação sensor de temp. e rtc

#include <OneWire.h>
#include <DallasTemperature.h>
#include <DS1307.h>

//----- Sistema de Armazenamento de Dados -----


//------------Nextion-----------
SoftwareSerial nextion(10, 11);// Nextion TX to pin 2 and RX to pin 3 of Arduino
Nextion myNextion(nextion, 9600); //create a Nextion object named myNextion using the nextion serial port @ 9600bps


//------------- Thread Coonf
ThreadController cpu;
Thread threadSensor;
Thread threadSensorpH;
Thread threadSensorOD;
Thread threadDisplay;

//---------------Sensores e Modulos------------
  #define ONE_WIRE_BUS 3 // Porta do pino de sinal do DS18B20
  DS1307 rtc(A4, A5); // Porta do rtc
  
  OneWire oneWire(ONE_WIRE_BUS);// Define uma instancia do oneWire para comunicacao com o sensor
   
  // Armazena temperaturas minima e maxima
  float tempMin = 999;
  float tempMax = 0;
   
  DallasTemperature sensors(&oneWire);
  DeviceAddress sensor1;

//----- Sensor de pH -----
    const int analogInPin = A2; 
    int sensorValue = 0; 
    unsigned long int avgValue; 
    float b;
    int buf[10],auxpH;

//----- Sistema de Armazenamento de Dados -----

#include <SD.h>
File collectData;

//--------- Variaveis de Botoes Data e Hora
int8_t type= -1;
int8_t buttonType = -1;
 //--------- Potenciometro
 int entradaDadospH = A0;
 int entradaDadosOD = A1;
 int valorDadospH = 0;
  int valorDadosOD = 0;
//---- Data e Hora RTC
uint8_t mesRTC = 10;
uint32_t anoRTC =  2017;
uint8_t diaRTC = 12;
uint8_t hourRTC = 14;
uint8_t minuteRTC = 25;
//----- Hora Monitoramento
uint8_t horaMoni = 2;
uint8_t minuMoni = 00;
//--- Variaveis Auxiliares
uint8_t auxMes = mesRTC;
uint32_t auxAno =  anoRTC;
uint8_t auxDia = diaRTC;
uint8_t auxHoraData = hourRTC;
uint8_t auxMinuData = minuteRTC;
uint8_t auxHoraMoni = horaMoni;
uint8_t auxMinuMoni = minuMoni;
//----------- Horas
int verificHour = 0;
int verificMin = 2;
int lastHour = 0;
int lastMin = 2;
//----------- Conversor de Interiros para Char (Display)
char buffer_temp[10] = {0};
char buffer_year[10] = {0};
char buffer_month[10] = {0};
char buffer_day[10] = {0};
char buffer_hour[10] = {0};
char buffer_minute[10] = {0};
char buffer_week[10] = {0};
uint8_t buffer_second =0;
char buffer_temperature[10] = {0};
char buffer_od[10] = {0};
char buffer_pH[10] = {0};


void setup() {
    Time t = rtc.getTime();
  // ----- inicializa o RTC e configuracoes iniciais
    rtc.halt(false);
    rtc.setDOW(FRIDAY);      //Define o dia da semana
    rtc.setTime(hourRTC, minuteRTC, 0);     //Define o horario
    rtc.setDate(diaRTC, mesRTC, anoRTC);   //Define o dia, mes e ano
    rtc.setSQWRate(SQW_RATE_1);
    rtc.enableSQW(true);
// --- Configuracoes das Threads    
    threadSensor.setInterval(1000);
    threadSensorpH.setInterval(1000);
    threadSensorOD.setInterval(1000);
    threadDisplay.setInterval(10);
    threadSensor.onRun(verificaTemperatura);
    threadSensorpH.onRun(verificapH);
    threadSensorOD.onRun(verificaOD);
    threadDisplay.onRun(iniciaValores);
    cpu.add(&threadSensor);
    cpu.add(&threadSensorpH);
    cpu.add(&threadSensorOD);
    cpu.add(&threadDisplay);
   //**** Inicia valores nas telas
    verificHour = t.hour;
    verificMin = t.min;
    auxMinuMoni = lastMin;
    auxHoraMoni = lastHour;
    Serial.begin(9600);
    SD.begin(53);
    myNextion.init(); // send the initialization commands for Page 0
    sensors.begin();
    
   Serial.print(sensors.getDeviceCount(), DEC);
    Serial.println(" sensores.");
    if (!sensors.getAddress(sensor1, 0)) 
       Serial.println("Sensores nao encontrados !"); 
    // Mostra o endereco do sensor encontrado no barramento
    Serial.print("Endereco sensor: ");
    mostra_endereco_sensor(sensor1);
   
}
void mostra_endereco_sensor(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // Adiciona zeros se necessário
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
  String message="";
  char buffer [50];
  boolean iniciarValores = true ;
void iniciaValores(){
   itoa(auxMes, buffer_month, 10);   
       myNextion.setComponentText("mes", buffer_month);
   itoa(auxDia, buffer_day, 10);   
       myNextion.setComponentText("dia", buffer_day);
   itoa(auxAno, buffer_year, 10);
        myNextion.setComponentText("ano", buffer_year);
   itoa(auxHoraMoni, buffer_hour, 10);
      myNextion.setComponentText("horaM", buffer_hour);
   itoa(auxMinuMoni, buffer_minute, 10);
      myNextion.setComponentText("minutoM", buffer_minute);    

}

void loop() {
    message = myNextion.listen(); //check for message
     verificaTxt();
       if(type>0){
         Serial.println(type);
          threadSensorpH.enabled = false;
          threadSensorOD.enabled = false;
        verificaBotao();
         if(buttonType>0){
         verificaAlteracao(); 
         }
       }
       cpu.run();
       Time t = rtc.getTime();
       float val = t.hour;
       float minutes = t.min;
       

    if(t.hour+" "+t.min==verificHour+" "+verificMin){
          verificMin = t.min+lastMin;
          verificHour = t.hour+lastHour;
        if(verificHour>23){
          verificHour = verificHour - 23;
        }
        if(verificMin>59){
            verificMin = verificMin-59;
            verificHour = verificHour +1;
          if(verificHour>23){
              verificHour = verificHour - 23;
          }
        }
        Serial.println("");        
        Serial.println(val);  
        storeData();      
    }
     
  }
  float tempC = 0;
 void verificaTemperatura(){
    sensors.requestTemperatures();
    tempC = sensors.getTempC(sensor1);
    
    itoa(tempC, buffer_temp, 10);

      myNextion.setComponentText("temperatura", buffer_temp);
     //--- Convert data
     String dia2 = rtc.getDateStr(); 
     dia2.toCharArray(buffer_day, 15);
     myNextion.setComponentText("diaPagInicial", buffer_day);
    
 }
  void verificapH(){
       for(int i=0;i<10;i++) 
       { 
        buf[i]=analogRead(analogInPin);
        delay(10);
       }
       for(int i=0;i<9;i++)
       {
        for(int j=i+1;j<10;j++)
        {
         if(buf[i]>buf[j])
         {
          auxpH=buf[i];
          buf[i]=buf[j];
          buf[j]=auxpH;
         }
        }
       }
        valorDadospH = analogRead(entradaDadospH);
       avgValue=0;
       for(int i=2;i<8;i++)
       avgValue+=buf[i];
       float pHVol=(float)avgValue*4/1024/6;
       float phValue = -5.75 * pHVol + 21.34;


       Serial.println(valorDadospH);
       itoa(phValue, buffer_pH, 10);
       myNextion.setComponentText("pH", buffer_pH);
       
      
  }
  void verificaOD(){
        valorDadosOD = analogRead(entradaDadosOD);
        valorDadosOD = (int)valorDadosOD*0.009852217;
        itoa(valorDadosOD, buffer_od, 10);
        myNextion.setComponentText("oxigenioD", buffer_od);
  }
 
void storeData(){
  collectData= SD.open("dados.txt", FILE_WRITE);
   if(collectData){
      collectData.print(rtc.getDateStr());
      collectData.print(", ");
      collectData.print(rtc.getTimeStr());
      collectData.print(" - Temperatura: ");
      verificaTemperatura();
      collectData.print(tempC);
      collectData.print("");
      collectData.print(".00 C ");
      collectData.print(" - pH: ");
      collectData.print(valorDadospH);
      collectData.print(" - Oxigenio Dissolvido: ");
      collectData.println(valorDadosOD);
      collectData.close();
      Serial.println("OK");
    }else
      Serial.println("ERRO");
}
 
void verificaAlteracao(){
  threadSensor.enabled = false;
  switch (type) {
    case 1:
         checkDay();
      break;
    case 2:
        checkMonth();
      break;
      case 3:
          checkYear();
      break;
      case 4:
       checkHour();
      break;
      case 5:
         checkMinute();
        break;
       case 6:
        checkHourMoni();
        break;
      case 7:
       checkMinuteMoni();
        break;
      default:;
      break;
  }
    threadSensor.enabled = true;
 }
 void checkDay(){
   if(buttonType ==1){
      buttonType = 0;
      verificaDiaUp();
   }
  if(buttonType ==2){
      buttonType = 0;
      verificaDiaDow();
    }
  if(buttonType ==3){
      buttonType = 0;
      rtc.setDate(auxDia, auxMes, auxAno);
      rtc.setTime(auxHoraData, auxMinuData, 0);     //Define o horario
  
   }
   itoa(auxDia, buffer_day, 10);
   myNextion.setComponentText("dia", buffer_day);
 }
 void checkMonth(){
  if(buttonType ==1){
      buttonType = 0;
      if(auxMes+1>12) auxMes = 1;  
      else auxMes++;
  }
  if(buttonType ==2){
      buttonType = 0;
      if(auxMes-1==0) auxMes = 12;
      else auxMes--;
   }
   if(buttonType ==3){
     buttonType = 0;
     rtc.setDate(auxDia, auxMes, auxAno);
     rtc.setTime(auxHoraData, auxMinuData, 0);     //Define o horario
  
   }
       itoa(auxMes, buffer_month, 10);   
       myNextion.setComponentText("mes", buffer_month);
 }
 void checkYear(){
   if(buttonType ==1){
     buttonType = 0;
     auxAno++;
   }
   if(buttonType ==2){
    buttonType = 0;
    auxAno--;
   }
   if(buttonType ==3){
     buttonType = 0;
     rtc.setDate(auxDia, auxMes, auxAno);
     rtc.setTime(auxHoraData, auxMinuData, 0);     //Define o horario
   }
   itoa(auxAno, buffer_year, 10);
        myNextion.setComponentText("ano", buffer_year);
 }
 void checkHour(){
   if(buttonType ==1){
     buttonType = 0;
     if(auxHoraData+1>23) auxHoraData = 00;  
     else auxHoraData++;
   }
   if(buttonType ==2){
      buttonType = 0;
      if(auxHoraData-1==-1) auxHoraData = 23;
      else auxHoraData--;
   }
   if(buttonType ==3){
     buttonType = 0;
     rtc.setDate(auxDia, auxMes, auxAno);
     rtc.setTime(auxHoraData, auxMinuData, 0);     //Define o horario
  
   }
      itoa(auxHoraData, buffer_hour, 10);
      myNextion.setComponentText("hora", buffer_hour);   
 }
 void checkMinute(){
 if(buttonType ==1){
     buttonType = 0;
     if(auxMinuData+1>59) auxMinuData = 00;  
     else auxMinuData++;
   }
   if(buttonType ==2){
      buttonType = 0;
      if(auxMinuData-1==-1) auxMinuData = 59;
      else auxMinuData--;
    }
    if(buttonType ==3){
      buttonType = 0;
      rtc.setDate(auxDia, auxMes, auxAno);
      rtc.setTime(auxHoraData, auxMinuData, 0);     //Define o horario
  
   }
    itoa(auxMinuData, buffer_minute, 10);
    myNextion.setComponentText("minuto", buffer_minute);   
 }
 void checkHourMoni(){
   if(buttonType ==4){
     buttonType = 0;
     if(auxHoraMoni+1>23) auxHoraMoni = 00;  
     else auxHoraMoni++;
   }
   if(buttonType ==5){
      buttonType = 0;
      if(auxHoraMoni-1==-1) auxHoraMoni = 23;
      else auxHoraMoni--;
   }
   if(buttonType==6){
       buttonType = 0;
       horaMoni =  auxHoraMoni;
       minuMoni= auxMinuMoni;
       
   }
      itoa(auxHoraMoni, buffer_hour, 10);
      myNextion.setComponentText("horaM", buffer_hour);   
 }
 void checkMinuteMoni(){
 if(buttonType ==4){
     buttonType = 0;
     if(auxMinuMoni+1>59) auxMinuMoni = 00;  
     else auxMinuMoni++;
   }
   if(buttonType ==5){
      buttonType = 0;
      if(auxMinuMoni-1==-1) auxMinuMoni = 59;
      else auxMinuMoni--;
    }
    if(buttonType==6){
       buttonType = 0;
       lastHour =  auxHoraMoni;
       lastMin= auxMinuMoni;
       
   }
    itoa(auxMinuMoni, buffer_minute, 10);
    myNextion.setComponentText("minutoM", buffer_minute);   
 }
void verificaDiaUp(){
    if((auxMes ==1 || auxMes ==8 || auxMes ==10  || auxMes ==12 || auxMes==7 || auxMes ==5 ||auxMes ==3)  && auxDia+1 >31) auxDia = 1;
      else if((auxMes ==4 || auxMes ==6 || auxMes ==9  || auxMes ==11 )  && auxDia+1 >30) auxDia = 1;
      else if(auxMes==2 ){
        if(auxAno%4==0){ 
          if(auxDia+1>29) auxDia = 1;
          else auxDia++;
          
          }
	else if(auxDia+1 > 28) auxDia = 1;
	else auxDia++;
      }
    else auxDia++;
  }
void verificaDiaDow(){
      if((auxMes ==1 || auxMes ==8 || auxMes ==10  || auxMes ==12 || auxMes ==7 || auxMes ==5 ||auxMes ==3) && auxDia-1 == 0) auxDia = 31;
      else if((auxMes ==4 || auxMes ==6 || auxMes ==9  || auxMes ==11 )  && auxDia-1  ==0) auxDia = 30;
      else if(auxMes==2 ){
      	   if(auxAno%4==0 && auxDia-1 == 0) auxDia = 29;
           else if(auxDia-1 == 0) auxDia = 28;
           else auxDia--;
      }else auxDia--;
    
}
void verificaBotao(){
    //------------- tela dataHora
    if(message=="65 2 7 1 ffff ffff ffff") buttonType = 1;
    else if(message=="65 2 8 1 ffff ffff ffff") buttonType = 2;
    else if(message=="65 2 a 1 ffff ffff ffff") buttonType = 3;
    //------------- tela monitoramento
    else if(message=="65 3 4 1 ffff ffff ffff") buttonType = 4;
    else if(message=="65 3 5 1 ffff ffff ffff") buttonType = 5;
    else if(message=="65 3 7 1 ffff ffff ffff") buttonType = 6;

  }
  void verificaTxt(){
    // tela dataHra
      if(message=="65 2 1 1 ffff ffff ffff") type = 1;
      else if(message=="65 2 2 1 ffff ffff ffff") type = 2;
      else if(message=="65 2 3 1 ffff ffff ffff") type = 3;
      else if(message=="65 2 4 1 ffff ffff ffff") type = 4;
      else if(message=="65 2 6 1 ffff ffff ffff") type = 5;
      // tela monitoramento
      else if(message=="65 3 1 1 ffff ffff ffff") type = 6;
      else if(message=="65 3 3 1 ffff ffff ffff") type = 7;

  }
  
