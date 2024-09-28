#ifndef RP2040_FREERTOS_IRQ_DISPLAY_H
#define RP2040_FREERTOS_IRQ_DISPLAY_H


#include <memory>
#include "i2c/PicoI2C.h"
#include "display/ssd1306os.h"
#include "display/ssd1306.h"

class Display {
public:
    Display(const std::shared_ptr<PicoI2C>& i2c_sp);
private:
    void display();
    static void task_display(void * params);

    TaskHandle_t mTaskHandle;
    ssd1306os mSSD1306;
};


#endif //RP2040_FREERTOS_IRQ_DISPLAY_H
