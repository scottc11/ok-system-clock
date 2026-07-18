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
#include "RotaryEncoder.h"
#include "HardwareTimer.h"
#include "Menu.h"
#include "menu_items.h"

TaskHandle_t th_main;
QueueHandle_t queue_main;
IWDG_HandleTypeDef hiwdg;

HardwareTimer timer8(TIM10);

I2C i2c(I2C1_SDA, I2C1_SCL, I2C::Instance::I2C_1, I2C::Mode::NonBlocking);

DigitalOut STATUS_LED(PD_2);
DigitalOut displayShutdown(DISPLAY_SHUTDOWN);
DigitalOut trigOut1(TRIG_OUT_1);
DigitalOut trigOut2(TRIG_OUT_2);

DigitalOut ledStartStop(LED_START_STOP);
DigitalOut ledReset(LED_RESET);

DigitalIn resetButton(BTN_RESET, PinMode::PullUp);
DigitalIn startStopButton(BTN_START_STOP, PinMode::PullUp);

IS31FL3246 leds(&i2c, IS31FL3246_ADDR_VCC);
Display display(&i2c, DISPLAY_SHUTDOWN);

Metronome metronome(EXT_CLOCK_IN, TIM_CHANNEL_3);

Menu menu(&menu_root);

RotaryEncoder encoder(ROTARY_ENCODER_A, ROTARY_ENCODER_B, ROTARY_ENCODER_BUTTON);
bool encoderPressed = false;
bool setupMode = false;

// Bit for each polled button. A set bit means the button is pressed (active LOW).
enum ButtonMask : uint8_t {
    BTN_MASK_RESET      = 1 << 0,
    BTN_MASK_START_STOP = 1 << 1,
};

// Last known pressed-state of all buttons, used for edge detection so that
// press/release handlers only run once per transition (not every poll).
uint8_t buttonState = 0;

// Reads the current pressed-state of all buttons into a single mask.
static uint8_t readButtons()
{
    uint8_t mask = 0;
    if (resetButton.read() == LOW)     mask |= BTN_MASK_RESET;
    if (startStopButton.read() == LOW) mask |= BTN_MASK_START_STOP;
    return mask;
}

// Polls the buttons and dispatches actions only on state changes.
static void handleButtons()
{
    uint8_t current  = readButtons();
    uint8_t changed  = current ^ buttonState;
    uint8_t pressed  = changed & current;      // bits that went 0 -> 1 (just pressed)
    uint8_t released = changed & buttonState;  // bits that went 1 -> 0 (just released)

    // Reset button press
    if (pressed & BTN_MASK_RESET) {
        if (encoderPressed) {
            setupMode = !setupMode;
            if (setupMode) {
                menu.jumpToMenuItem(M_ROOT);
                display.drawString(menu.getActiveItemText());
            } else {
                menu.jumpToMenuItem(M_METRONOME_BPM);
                display.drawString(menu.getActiveItemText());
            }
        }
    }

    // Reset button release
    if (released & BTN_MASK_RESET) {
    }

    // Start/stop button press
    if (pressed & BTN_MASK_START_STOP) {
        if (setupMode) {
            menu.handleSelect();
        } else {
            if (metronome.running) {
                metronome.stop();
                ledStartStop.write(HIGH);
            } else {
                metronome.start();
                ledStartStop.write(LOW);
            }
        }
    }

    // Start/stop button release
    if (released & BTN_MASK_START_STOP) {
    }

    buttonState = current;
}

/**
 * @brief Callback for the metronome PQN pulse.
 * @note Executed in interrupt context, put all timing critical gpio operations here.
 * @param pulse 
 */
void ppqnCallback(uint8_t pulse)
{   
    if (pulse == 0)
    {
        trigOut1.write(HIGH);
    } else {
        trigOut1.write(LOW);
    }

    dispatch_event_isr(Event::METRONOME_PULSE); // update the UI
}

void stepCallback(uint16_t step)
{
    // ledStartStop.toggle();
}

// occurs in interrupt context
void encoderRotateCallback(uint8_t direction)
{
    dispatch_event_isr(Event::ENCODER_ROTATE);
}

// occurs in interrupt context
void encoderPressCallback()
{
    encoderPressed = true;
    dispatch_event_isr(Event::ENCODER_PRESS);
}

void encoderReleaseCallback()
{
    encoderPressed = false;
    dispatch_event_isr(Event::ENCODER_RELEASE);
}

void menuJumpCallback(MenuItem *item)
{
    display.drawString(item->text);
}

void timerOverflowCallback()
{
    dispatch_event_isr(Event::UPDATE_DISPLAY);
}

void taskMain(void *pvParameters)
{
    // Task variables
    queue_main = xQueueCreate(32, sizeof(uint32_t));
    uint32_t event_id = 0x0; // the data that will be passed from functions putting things in the queue

    i2c.init();
    leds.init();
    display.init();
    metronome.init();
    
    timer8.init(8, 1000);
    timer8.attachOverflowCallback(callback(timerOverflowCallback));
    timer8.setOverflowFrequency(30);
    timer8.start();

    display.drawString("OK200");
    display.update();

    STATUS_LED.write(1);

    encoder.attachRotateCallback(encoderRotateCallback);
    encoder.attachPressCallback(encoderPressCallback);
    encoder.attachReleaseCallback(encoderReleaseCallback);

    metronome.attachPPQNCallback(ppqnCallback);
    metronome.attachStepCallback(stepCallback);
    metronome.start();

    menu.jumpToMenuItem(M_METRONOME_BPM);
    display.drawFloat(metronome.getBPM());

    while (1)
    {
        xQueueReceive(queue_main, &event_id, portMAX_DELAY);
        HAL_IWDG_Refresh(&hiwdg);

        switch (event_id)
        {
        case Event::UPDATE_DISPLAY:
            display.update();
            handleButtons();
            break;

        case Event::METRONOME_PULSE:
            if (metronome.pulse == 0) {
                leds.setChannelPWM(14, 10);
            } else {
                leds.setChannelPWM(14, 0);
            }
            break;

        case Event::ENCODER_ROTATE:
            menuHandler(encoder.direction);
            break;

        case Event::ENCODER_PRESS:
            break;

        case Event::ENCODER_RELEASE:
            break;

        default:
            break;
        }
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
    Metronome::RouteOverflowCallback(htim);
    HardwareTimer::RoutePeriodElapsedCallback(htim);
}

void OK_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    Metronome::RouteCaptureCallback(htim);
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