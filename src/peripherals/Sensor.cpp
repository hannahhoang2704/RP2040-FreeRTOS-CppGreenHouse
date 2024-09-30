#include "Sensor.h"
#include "Logger.h"


using namespace std;

namespace Sensor {

    CO2::CO2(const shared_ptr <ModbusClient>& modbusClient) :
            mGMP252_low(modbusClient, GMP252_s.servAddr, GMP252_s.regAddrCO2),
            mGMP252_high(modbusClient, GMP252_s.servAddr, GMP252_s.regAddrCO2 + 1) {}

    float CO2::update() {
        mCO2.u32 = mGMP252_low.read();
        mCO2.u32 += mGMP252_high.read() << 16;
        return mCO2.f;
    }

    float CO2::value() const {
        return mCO2.f;
    }

    ///

    Humidity::Humidity(const std::shared_ptr<ModbusClient>& modbusClient) :
            mHMP60(modbusClient, HMP60_s.servAddr, HMP60_s.regAddrRelHum) {}

    float Humidity::update() {
        mRelHum = mHMP60.read();
        mRelHum /= 10;
        return mRelHum;
    }

    float Humidity::value() const {
        return mRelHum;
    }

    ///

    Temperature::Temperature(const shared_ptr <ModbusClient> &modbusClient) :
            mGMP252_low(modbusClient, GMP252_s.servAddr, GMP252_s.regAddrTemp),
            mGMP252_high(modbusClient, GMP252_s.servAddr, GMP252_s.regAddrTemp + 1),
            mHMP60(modbusClient, HMP60_s.servAddr, HMP60_s.regAddrTemp) {}

    float Temperature::update_GMP252() {
        mTempGMP252.u32 = mGMP252_low.read();
        mTempGMP252.u32 += mGMP252_high.read() << 16;
        return mTempGMP252.f;
    }

    float Temperature::update_HMP60() {
        mTempHMP60 = mHMP60.read();
        mTempHMP60 /= 10;
        return mTempHMP60;
    }

    float Temperature::update_all() {
        return (update_GMP252() + update_HMP60()) / 2;
    }

    float Temperature::value_GMP252() const {
        return mTempGMP252.f;
    }

    float Temperature::value_HMP60() const {
        return mTempHMP60;
    }

    float Temperature::value_average() const {
        return (mTempGMP252.f + static_cast<float>(mTempHMP60)) / 2;
    }



    PressureSensor::PressureSensor(std::shared_ptr<PicoI2C> &i2c_sp):
    mI2C(i2c_sp){
        mDevAddr = SDP610_s.regAddrPressure;
    }

    int PressureSensor::update_SDP610() {
        mI2C->transaction(mDevAddr, trigger_measurement_cmd, 1, mBuffer, buffer_len);
        vTaskDelay(100);
        mPressure_value = ((mBuffer[0] << 8) | mBuffer[1]) / differential_pressure * correction_factor;
        mPressure_value = (mPressure_value <= SDP610_s.minPressure) ? 0 : (mPressure_value >= SDP610_s.maxPressure) ? 0 : mPressure_value;
        return mPressure_value;
    }

    int PressureSensor::value_SDP610() const {
        return mPressure_value;
    }
}
