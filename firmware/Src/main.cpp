#include "main.h"
#include "globals.h"
#include "random.h"
#include "I2C.h"
#include "HardwareTimer.h"
#include "DigitalOut.h"
#include "DigitalIn.h"
#include "AnalogIn.h"
#include "AnalogOut.h"
#include "InterruptIn.h"
#include "IS31FL3246.h"
#include "Metronome.h"

TaskHandle_t th_main;
QueueHandle_t queue_main;
IWDG_HandleTypeDef hiwdg;

I2C i2c(I2C1_SDA, I2C1_SCL, I2C::Instance::I2C_1, I2C::Mode::NonBlocking);

DigitalOut STATUS_LED(PD_2);
DigitalOut displayShutdown(DISPLAY_SHUTDOWN);
DigitalOut trigOut1(TRIG_OUT_1);
DigitalOut trigOut2(TRIG_OUT_2);

DigitalOut ledStartStop(LED_START_STOP);
DigitalOut ledReset(LED_RESET);

IS31FL3246 leds(&i2c, IS31FL3246_ADDR_GND);
Display display(&i2c, DISPLAY_SHUTDOWN);

// TIM4 overflow determines the BPM (overflow event ticks a pulse, 24 pulses per quarter note)
// TIM2 handles external clock input via input capture
// TIM2 divides capture event by 24 and sets TIM4 overflow to the result

void taskMain(void *pvParameters)
{
    // Task variables
    queue_main = xQueueCreate(32, sizeof(uint32_t));
    uint32_t event_id = 0x0; // the data that will be passed from functions putting things in the queue

    i2c.init();
    leds.init();
    display.init();

    STATUS_LED.write(1);

    while (1)
    {
        HAL_Delay(100);
        HAL_IWDG_Refresh(&hiwdg);
        display.drawString("HELLO!");
        display.update();

        // xQueueReceive(queue_main, &event_id, portMAX_DELAY);
        // HAL_IWDG_Refresh(&hiwdg);

        // switch (event_id)
        // {
        // default:
        //     break;
        // }
    }
}

int main(void)
{
    HAL_Init();

    SystemClock_Config();

    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
    hiwdg.Init.Reload = 500; // 1 second timeout
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
    {
        OK_ERROR_HANDLER(HAL_ERROR, "Watchdog initialization failed");
    }

    HAL_Delay(5);
    // ADC sample rate should be at least 2x the speed of the multiplexer switching rate
    AnalogIn::initialize(20000); // 20Khz ADC sample rate (@ 100KHz, there is a bug causing the gpio expander interrupt BUTTONS_INT to fail... not sure why)
    HAL_Delay(5);
    InterruptIn::initialize();
    DigitalOut::initialize();
    HAL_Delay(90);

    ok_random_seed(HAL_GetTick());

    xTaskCreate(taskMain, "taskMain", 512, NULL, 1, &th_main);

    xTaskCreate(task_I2C_manager, "I2C manager", 256, NULL, 3, &th_i2c_manager);

    vTaskStartScheduler();

    while (1)
    {
    }
}

void OK_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    HardwareTimer::RoutePeriodElapsedCallback(htim);
}

void OK_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        
    }
}

extern "C" void OK_I2C_MANAGER_WHILE_LOOP_START(TickType_t *last_wake_time)
{
    UNUSED(last_wake_time);
    STATUS_LED.toggle();
}

HAL_StatusTypeDef OK_ERROR_HANDLER(HAL_StatusTypeDef error, const char *msg)
{
    UNUSED(msg);
    switch (error)
    {
    case HAL_ERROR:
        // STATUS_LED.write(1);
        break;
    default:
        break;
    }
    return error;
}