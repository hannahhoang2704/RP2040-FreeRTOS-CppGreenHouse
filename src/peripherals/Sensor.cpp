#include "Sensor.h"


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

    float CO2::f() const {
        return mCO2.f;
    }

    uint32_t CO2::u32() const {
        return mCO2.u32;
    }

    ///

    Humidity::Humidity(const std::shared_ptr<ModbusClient>& modbusClient) :
            mHMP60(modbusClient, HMP60_s.servAddr, HMP60_s.regAddrRelHum) {}

    float Humidity::update() {
        mRelHum = mHMP60.read();
        mRelHum /= 10;
        mRH.u16 = mHMP60.read();
        mRH.f /= 10;
        return mRH.f;
    }

    float Humidity::f() const {
        return mRelHum;
    }

    uint32_t Humidity::u32() const {
        return mRH.u32;
    }

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
        return (mTempGMP252.f + mTempHMP60) / 2;
    }
}
