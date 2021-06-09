#include "SensObj.h"


long SensObj::_readNoise( ){
  long sum = 0;
  for(int i=0; i<32; i++)
  {
      sum += analogRead( _SoundSensorPin );
  }

  sum >>= 5;
  delay(10);
  return sum;
}


void SensObj::_resetNoiseSum(){
  _NoiseSum = 0;
  _nNoiseMeasurements = 0;
  _maxNoise = 0;
}


void SensObj::addNoiseMeasurement(){
  long myNoise;
  myNoise = _readNoise();
  if (myNoise > _maxNoise) _maxNoise = myNoise;
  _NoiseSum += _readNoise();
  _nNoiseMeasurements++;
}

float SensObj::_getMeanNoise(){
  if (_nNoiseMeasurements > 0)
    return float(_NoiseSum) / float( _nNoiseMeasurements );
  else
    return 0.0;
}



void SensObj::start( int aPinSoundSensor, int aPinAirQuality ){
  // try to start Enviromental Shield
  _envShieldConnected = ENV.begin();
  if( _SoundSensorPin ) { Serial.println("OK: Env shield found."); }
  else { Serial.println("ERROR: Env shield not found!"); }

  // try to start Sound Sensor on pin aPinSoundSensor
  _SoundSensorPin = aPinSoundSensor;
  _soundSensorConnected = true; //  TODO: Change to check if connected)  
  if( _soundSensorConnected ) { Serial.println("OK: Sound sensor found."); }
  else { Serial.println("ERROR: Sound sensor not found!"); }
    
  // try to start VOC and CO2 
  _vocSensorConnected = _VOC_CO2_setup();   
  if( _vocSensorConnected )  { Serial.println("OK: VOC and CO2 sensor found."); }
  else { Serial.println("ERROR: VOC and CO2 sensor not found!"); }

  // try to start Air Quality Sensor on pin aPinAirQuality
  _airQSensorConnected = _AirQualitySensor.start(aPinAirQuality);  
  // BUG: RETURNS TRUE, EVEN IF NOT CONNECTED !!!
  if( _airQSensorConnected ) { Serial.println("OK: Air Quality Sensor sensor found."); }
  else { Serial.println("ERROR: Air Quality Sensor sensor not found!"); }

  // try to laser pm2.5 sensor HM3301
  _ppmSensorConnected = !( dustSensor.init() ); // if dustSensor.init() == true, then problems. see example http://wiki.seeedstudio.com/Grove-Laser_PM2.5_Sensor-HM3301/
  if( _ppmSensorConnected ) { Serial.println("OK: Dust sensor found."); }
  else { Serial.println("ERROR: Dust sensor not found!"); }
}

bool SensObj::_VOC_CO2_setup() {
  bool err_status;
  s16 err;
  u16 scaled_ethanol_signal, scaled_h2_signal;
  Serial.begin(115200);
  Serial.println("serial start!!");

 /*For wio link!*/
  #if defined(ESP8266)
          pinMode(15,OUTPUT);
          digitalWrite(15,1);
          Serial.println("Set wio link power!");
          delay(500);
  #endif
  /*Init module,Reset all baseline,The initialization takes up to around 15 seconds, during which
all APIs measuring IAQ(Indoor air quality ) output will not change.Default value is 400(ppm) for co2,0(ppb) for tvoc*/
  while (sgp_probe() != STATUS_OK) {
         Serial.println("SGP failed");
         //while(1);
         return false;
    }
    /*Read H2 and Ethanol signal in the way of blocking*/
    err = sgp_measure_signals_blocking_read(&scaled_ethanol_signal, &scaled_h2_signal);
    err_status = (err == STATUS_OK);
    if  (err_status){
        Serial.println("get ram signal!");
        
    } else {
         Serial.println("error reading signals"); 
    }

     err = sgp_iaq_init();
    return err_status;
     //
}


bool SensObj::_readVOC_CO2() {
  s16 err=0;
  err = sgp_measure_iaq_blocking_read(&_tvoc_ppb, &_co2_eq_ppm);
  return (err == STATUS_OK);
       /*
    return true
      Serial.print("tVOC  Concentration:");
      Serial.print(tvoc_ppb);
      Serial.println("ppb");
      
      Serial.print("CO2eq Concentration:");
      Serial.print(co2_eq_ppm);
      Serial.println("ppm");

   return false:
      // Serial.println("error reading IAQ values\n"); */
}

// code from http://wiki.seeedstudio.com/Grove-Air_Quality_Sensor_v1.3/
// ToDo: pass A2 as parameter !!!!
bool SensObj::_readAirQualSensor(){
  return _AirQualitySensor.start(A2);
}


void SensObj::addMeasurement(String& aString){
  if(_envShieldConnected)  _addMeasurementEnvShield( aString );
  if(_soundSensorConnected) _addMeasurementSound( aString );
  if(_airQSensorConnected) _addMeasurementAirQuality( aString );
  if(_ppmSensorConnected)  _addMeasurementPPM( aString );  //e2c
  if(_vocSensorConnected)  _addMeasurementVoc_CO2( aString );  //e2c
}

void SensObj::_addMeasurementEnvShield(String& aString){
   // add each measurement value 
      aString += ",\"temp\":"     + String(ENV.readTemperature() - _tempCorrection ,2);
      aString += ",\"hm\":"      + String(ENV.readHumidity(), 1);
  //  aString += ",\"pressure\":" + String(ENV.readPressure());
      aString += ",\"lux\":"      + String(ENV.readIlluminance());
  //  aString += ",\"uva\":"      + String(ENV.readIlluminance());
  //  aString += ",\"uvb\":"      + String(ENV.readUVB());
      aString += ",\"UV\":"       + String(ENV.readUVIndex());
}

void SensObj::_addMeasurementSound(String& aString){
      // TO DO: CALC MEAN, MAX, MIN LAST 20min
      aString += ",\"mN\":" + String( _getMeanNoise(), 0 );
      aString += ",\"mxN\":" + String( _maxNoise );
      _resetNoiseSum();
}

void SensObj::_addMeasurementVoc_CO2(String& aString){
    if (_readVOC_CO2()) {
      aString += ",\"TVOC\":" + String( _tvoc_ppb );
      aString += ",\"eCO2\":" + String( _co2_eq_ppm );
    }
}


/*
  0:AirQualitySensor::FORCE_SIGNAL
  1:AirQualitySensor::HIGH_POLLUTION
  2:AirQualitySensor::LOW_POLLUTION
  3:AirQualitySensor::FRESH_AIR
 */
void SensObj::_addMeasurementAirQuality(String& aString){
  int my_quality;
  my_quality = _AirQualitySensor.slope();
  
  aString += ",\"AirQV\":" + String( _AirQualitySensor.getValue() ); // air quality as integer
  aString += ",\"AirQ\":"  + String( my_quality );                   // air quality as type. 0 bad, 3 good

}


void SensObj::_addMeasurementPPM(String& aString)
{
    if (dustSensor.read_sensor_value(HM330X_buf, 29)) {
        Serial.println("HM330X read result failed!!!");
    }else{
      if (HM330X_parse_result_value(HM330X_buf)) {
        HM330X_parse_result(HM330X_buf, aString);
      }
    }
}

    bool SensObj::HM330X_parse_result_value(uint8_t *data) {
      if (NULL == data) {
          //return ERROR_PARAM;
          return false;
      }
      //for (int i = 0; i < 28; i++) {
      //    SERIAL_OUTPUT.print(data[i], HEX);
      //    SERIAL_OUTPUT.print("  ");
      //    if ((0 == (i) % 5) || (0 == i)) {
      //        SERIAL_OUTPUT.println("");
      //    }
      //}turn
      uint8_t sum = 0;
      for (int i = 0; i < 28; i++) {
          sum += data[i];
      }
      if (sum != data[28]) {
        Serial.println("HM330X wrong checkSum!!!!");
        return false;
      }
      //SERIAL_OUTPUT.println("");
      //return NO_ERROR;
      return true;
    }

      /*parse buf with 29 uint8_t-data*/
    int SensObj::HM330X_parse_result(uint8_t *data, String& aString) {
      uint16_t value = 0;
      if (NULL == data)
          return ERROR_PARAM;
      for (int i = 1; i < 8; i++) {
          value = (uint16_t) data[i * 2] << 8 | data[i * 2 + 1];
          if (i==5) aString += ",\"PM1.0\":" + String( value ); 
          if (i==6) aString += ",\"PM2.5\":" + String( value ); 
          if (i==7) aString += ",\"PM10\":" + String( value ); 
         // print_result(str[i - 1], value);  
      }
  
      return NO_ERROR;
    }
