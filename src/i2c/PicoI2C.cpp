//
// Created by Keijo Länsikunnas on 10.9.2024.
//
#include <mutex>
#include "pico/stdlib.h"
#include "PicoI2C.h"

#define I2C0_SDA_PIN 16
#define I2C0_SCL_PIN 17

#define I2C1_SDA_PIN 14
#define I2C1_SCL_PIN 15

PicoI2C *PicoI2C::i2c0_instance{nullptr};
PicoI2C *PicoI2C::i2c1_instance{nullptr};
void PicoI2C::i2c0_irq(void) {
    if(i2c0_instance) i2c0_instance->isr();
}

void PicoI2C::i2c1_irq(void) {
    if(i2c1_instance) i2c1_instance->isr();
}

PicoI2C::PicoI2C(uint bus_nr, uint speed) {
    int scl = I2C0_SCL_PIN;
    int sda = I2C0_SDA_PIN;
    switch(bus_nr) {
        case 0:
            i2c = i2c0;
            irqn = I2C0_IRQ;
            break;
        case 1:
            i2c = i2c1;
            irqn = I2C1_IRQ;
            scl = I2C1_SCL_PIN;
            sda = I2C1_SDA_PIN;
            break;
        default:
            panic("Invalid I2C bus number\n");
            break;
    }
    gpio_init(scl);
    gpio_pull_up(scl);
    gpio_init(sda);
    gpio_pull_up(sda);
    i2c_init(i2c, speed);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    irq_set_enabled(irqn, false);
    irq_set_exclusive_handler(irqn, bus_nr ? i2c1_irq : i2c0_irq);
}

#if 0
uint PicoI2C::write(uint8_t addr, const uint8_t *buffer, uint length) {
    std::lock_guard<Fmutex> exclusive(access);
    task = xTaskGetCurrentTaskHandle();
    uint count = 0;
    // setup transfer

    // enable interrupts
    irq_set_enabled(irqn, true);

    count = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));
    // if count != length transaction failed
    irq_set_enabled(irqn, false);

    return count;
}
#endif

#if 1
uint PicoI2C::write(uint8_t addr, const uint8_t *buffer, uint length) {
{
    std::lock_guard<Fmutex> exclusive(access);
    i2c->hw->enable = 0;
    i2c->hw->tar = addr;
    i2c->hw->enable = 1;

    bool abort = false;
    bool timeout = false;

    uint32_t abort_reason = 0;
    uint byte_ctr;

    i2c->restart_on_next = false;

    for (byte_ctr = 0; byte_ctr < length; ++byte_ctr) {
        bool first = byte_ctr == 0;
        bool last = byte_ctr == length - 1;

        i2c->hw->data_cmd =
                bool_to_bit(first && i2c->restart_on_next) << I2C_IC_DATA_CMD_RESTART_LSB |
                bool_to_bit(last) << I2C_IC_DATA_CMD_STOP_LSB |
                *buffer++;
        while(!last && i2c_get_write_available(i2c)==0){
            vTaskDelay(0);
        }
    }
    while(!(i2c->hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_STOP_DET_BITS)) {
        vTaskDelay(1);
    }
    timeout = i2c->hw->clr_stop_det;
#if 0
        // Wait until the transmission of the address/data from the internal
        // shift register has completed. For this to function correctly, the
        // TX_EMPTY_CTRL flag in IC_CON must be set. The TX_EMPTY_CTRL flag
        // was set in i2c_init.
        do {
            //if (timeout_check) {
            //    timeout = timeout_check(ts);
            //    abort |= timeout;
            //}
            tight_loop_contents();
        } while (!timeout && !(i2c->hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_TX_EMPTY_BITS));
#endif
    }
    return 0;
}
#endif
uint PicoI2C::read(uint8_t addr, uint8_t *buffer, uint length)
{
    return 0;
}

uint PicoI2C::transaction(uint8_t addr, const uint8_t *wbuffer, uint wlength, uint8_t *rbuffer, uint rlength)
{
    return 0;
}


#define done true
uint32_t byte_count = 0;

void PicoI2C::isr() {
    BaseType_t hpw = pdFALSE;
    // handle the transaction here
    // notify if we are done
    if(done) xTaskNotifyFromISR(task, byte_count, eSetValueWithOverwrite, &hpw);
    portYIELD_FROM_ISR(hpw);
}
