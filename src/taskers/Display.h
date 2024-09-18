#ifndef RP2040_FREERTOS_IRQ_DISPLAY_H
#define RP2040_FREERTOS_IRQ_DISPLAY_H


#include <memory>
#include "i2c/PicoI2C.h"
#include "display/ssd1306os.h"

class Display {
public:
    Display(std::shared_ptr<PicoI2C> i2c_sp);
private:
    void display();
    static void task_display(void * params) {
        auto object_ptr = static_cast<Display *>(params);
        object_ptr->display();
    }

    TaskHandle_t mTaskHandle;
    ssd1306os mDisplay;
};


#endif //RP2040_FREERTOS_IRQ_DISPLAY_H
