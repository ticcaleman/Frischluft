/*
  SensObj.h - Library for taking some sensor measurements.
  Created by Harutiun Alepoglian, January, 2020.
  Released into the public domain.
*/
#ifndef SensObj_h
#define SensObj_h


#define UNITS_VOC "ppb"
#define UNITS_CO2 "ppm"

#define UNITS_PM1_0 "ug/m3"
#define UNITS_PM2_5 "ug/m3"
#define UNITS_PM10  "ug/m3"
                    
#define mNoise "ug/m3"
#define mxNoise "ug/m3"
#define UNITS_PM10  "ug/m3"
#define UNITS_PM1_0 "ug/m3"
#define UNITS_PM2_5 "ug/m3"
#define UNITS_PM10  "ug/m3"
                    






#include <Arduino.h>

// for Enviromental Shield
#include <Arduino_MKRENV.h>


// for VOC and eCO2 sensor
#include "sensirion_common.h"
#include "sgp30.h"


// fork of "Air_Quality_Sensor.h" from https://github.com/Seeed-Studio/Grove_Air_quality_Sensor
// modified to remove the parameters when declaring the object
// need to call the funcion >start< after start
// start returns true, everything OK, or false, not OK
#include "Air_Quality_Sensor_FORK.h"


// for 1, 2.5 and 10ug/m3 dust measure
// see http://wiki.seeedstudio.com/Grove-Laser_PM2.5_Sensor-HM3301/
#include <Seeed_HM330X.h>



class SensObj 
{
  private:

    /*
     * Correction values
     * 
     */
    float _tempCorrection = 2.5;

    
    // is sensor X avaible
    bool _envShieldConnected = false;
    bool _soundSensorConnected = false;
    bool _ppmSensorConnected = false;
    bool _vocSensorConnected = false;
    bool _airQSensorConnected = false;

    // Air Quality 
    AirQualitySensor  _AirQualitySensor;    // da error si lo meto dentro de la parte private...
    int _airQualityPin = 2;
    bool _readAirQualSensor();

    // VOC & CO2    
    u16 _tvoc_ppb, _co2_eq_ppm;
    bool _VOC_CO2_setup();
    bool _readVOC_CO2();

    // Sound Sensor
    int _SoundSensorPin = -1;
    unsigned long _NoiseSum = 0;
    unsigned long _nNoiseMeasurements = 0;
    long _maxNoise = 0;
    float _getMeanNoise();
    void _resetNoiseSum();
    long _readNoise( );

    // Dust sensor
    HM330X dustSensor;
    uint8_t HM330X_buf[30];
    bool HM330X_parse_result_value(uint8_t *data);
    int HM330X_parse_result(uint8_t *data, String& aString);


 

    // add measurements
    void _addMeasurementEnvShield(String& aString);
    void _addMeasurementSound(String& aString);
    void _addMeasurementVoc_CO2(String& aString);
    void _addMeasurementAirQuality(String& aString);
    void _addMeasurementPPM(String& aString);

  public:
    void start( int aPinSoundSensor, int aPinAirQuality );
    void addMeasurement(String& aString);
    void addNoiseMeasurement();
};

#endif
