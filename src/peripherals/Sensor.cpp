#include "Sensor.h"


using namespace std;

namespace Sensor {

    GMP252::GMP252(const shared_ptr <ModbusClient>& modbusClient) :
            mCO2_low(modbusClient, GMP252_s.servAddr, GMP252_s.regAddrCO2),
            mCO2_high(modbusClient, GMP252_s.servAddr, GMP252_s.regAddrCO2 + 1),
            mTemp_low(modbusClient, GMP252_s.servAddr, GMP252_s.regAddrTemp),
            mTemp_high(modbusClient, GMP252_s.servAddr, GMP252_s.regAddrTemp + 1) {}

    float GMP252::update_co2() {
        mCO2.u32 = mCO2_low.read();
        mCO2.u32 += mCO2_high.read() << 16;
        return mCO2.f;
    }

    float GMP252::update_temperature() {
        mTemp.u32 = mTemp_low.read();
        mTemp.u32 += mTemp_high.read() << 16;
        return mTemp.f;
    }

    float GMP252::co2() const {
        return mCO2.f;
    }

    float GMP252::temperature() const {
        return mTemp.f;
    }


    HMP60::HMP60(const std::shared_ptr<ModbusClient>& modbusClient) :
            mRelHum_mr(modbusClient, HMP60_s.servAddr, HMP60_s.regAddrRelHum),
            mTemp_mr(modbusClient, HMP60_s.servAddr, HMP60_s.regAddrTemp) {}

    float HMP60::update_relHum() {
        mRelHum = mRelHum_mr.read();
        mRelHum /= 10;
        return mRelHum;
    }

    float HMP60::update_temperature() {
        mTemp = mTemp_mr.read();
        mTemp /= 10;
        return mTemp;
    }

    float HMP60::relHum() const {
        return mRelHum;
    }

    float HMP60::temperature() const {
        return mRelHum;
    }
}
