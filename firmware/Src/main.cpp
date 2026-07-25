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
#include "ClockOutput.h"
#include "MIDI.h"
#include "CAN.h"
#include "uart.h"

TaskHandle_t th_main;
QueueHandle_t queue_main;
IWDG_HandleTypeDef hiwdg;

HardwareTimer timer8(TIM10);

I2C i2c(I2C1_SDA, I2C1_SCL, I2C::Instance::I2C_1, I2C::Mode::NonBlocking);

CAN can_bus(CAN2, CAN_RX, CAN_TX);

DigitalOut STATUS_LED(PD_2);
DigitalOut displayShutdown(DISPLAY_SHUTDOWN);

DigitalOut transport_ppqn1(TRANSPORT_PPQN_1);
DigitalOut transport_ppqn24(TRANSPORT_PPQN_24);
DigitalOut transport_reset(TRANSPORT_RESET);
DigitalOut transport_startStop(TRANSPORT_START_STOP, 1); // default to "running"

ClockOutput output1(TRIG_OUT_1);
ClockOutput output2(TRIG_OUT_2);

DigitalOut ledStartStop(LED_START_STOP);
DigitalOut ledReset(LED_RESET);

DigitalIn resetButton(BTN_RESET, PinMode::PullUp);
DigitalIn startStopButton(BTN_START_STOP, PinMode::PullUp);

IS31FL3246 leds(&i2c, IS31FL3246_ADDR_VCC);
Display display(&i2c, DISPLAY_SHUTDOWN);

Metronome metronome(EXT_CLOCK_IN, TIM_CHANNEL_3);

MIDI midi(&huart1);

Menu menu(&menu_root);

RotaryEncoder encoder(ROTARY_ENCODER_A, ROTARY_ENCODER_B, ROTARY_ENCODER_BUTTON);
bool encoderPressed = false;
bool setupMode = false;
bool queueReset = false;

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
        queueReset = true;
        if (setupMode)
        {
            // exit setup mode
            setupMode = false;
            display.drawFloat(metronome.getBPM());
        }

        if (encoderPressed) {
            setupMode = !setupMode;
            if (setupMode) {
                syncMenuValuesRecursive(menu_root);
                menu.jumpToMenuItem(M_ROOT);
                display.drawString(menu.getActiveItemText());
            } else {
                display.drawFloat(metronome.getBPM());
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
            display.drawString(menu.getActiveItemText());
        } else {
            if (metronome.running) {
                metronome.stop();
                transport_startStop.write(0);
                midi.sendClockStop();
                ledStartStop.write(HIGH);
            } else {
                metronome.start();
                transport_startStop.write(1);
                midi.sendClockStart();
                ledStartStop.write(LOW);
            }
        }
    }

    // Start/stop button release
    if (released & BTN_MASK_START_STOP) {
    }

    buttonState = current;
}

float calculateBPM()
{
    static uint32_t last_call_time = 0;
    static float last_bpm = 0.0f;

    uint32_t current_time = HAL_GetTick(); // Get current time in ms
    if (last_call_time != 0)
    {
        uint32_t delta = current_time - last_call_time;
        // If nonzero and reasonable timing, compute BPM
        if (delta > 0)
        {
            // stepCallback is called once per step.
            // Convert ms per step to BPM:
            // BPM = 60000 ms / delta
            last_bpm = 60000.0f / (float)delta;
            // Optionally, do something with last_bpm, e.g., display.updateBPM(last_bpm);
            // For demonstration, you might display it:
            // display.drawFloat(last_bpm);
        }
    }
    last_call_time = current_time;
    return last_bpm;
}

/**
 * @brief Callback for the metronome PQN pulse.
 * @note Executed in interrupt context, put all timing critical gpio operations here.
 * @param pulse 
 */
void ppqnCallback(uint8_t pulse)
{   
    output1.update(pulse);
    output2.update(pulse);
    transport_ppqn24.write(1);
    
    if (pulse == 1) {
        transport_ppqn1.write(0);
    }

    midi.sendClockTick();

    if (queueReset) {
        transport_reset.write(1);
        queueReset = false;
    } else {
        transport_reset.write(0);
    }

    dispatch_event_isr(Event::METRONOME_PULSE); // update the UI
}

void stepCallback(uint16_t step)
{
    transport_ppqn1.write(1);
    dispatch_event_isr(Event::METRONOME_STEP);
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

void timerOverflowCallback()
{
    dispatch_event_isr(Event::UPDATE_DISPLAY);
}

void MIDIClockTickCallback()
{
    if (metronome.mode == Metronome::Mode::MIDI) {
        metronome.tick();
    }
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

    can_bus.init();
    
    uart_init();
    midi.attachClockTickCallback(callback(MIDIClockTickCallback));

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
    metronome.setMode(Metronome::Mode::INTERNAL);
    metronome.start();

    display.drawFloat(metronome.getBPM());
    float bpm = 0.0f;
    uint16_t can_data = 100;
    HAL_StatusTypeDef can_status;
    while (1)
    {
        xQueueReceive(queue_main, &event_id, portMAX_DELAY);
        HAL_IWDG_Refresh(&hiwdg);

        switch (event_id)
        {
        case Event::UPDATE_DISPLAY:
            display.update();
            handleButtons();
            can_status = can_bus.transmit(OK_CAN_ID_SYSTEM_CLOCK, (uint8_t *)&can_data, 2, false);
            break;

        case Event::METRONOME_PULSE:
            leds.setChannelPWM(14, output1.isTriggered(metronome.pulse) ? 10 : 0);
            leds.setChannelPWM(16, output2.isTriggered(metronome.pulse) ? 10 : 0);
            break;

        case Event::METRONOME_STEP:
            bpm = calculateBPM();
            if (metronome.mode == Metronome::Mode::EXTERNAL || metronome.mode == Metronome::Mode::MIDI) {
                metronome.setBPM(bpm);
                if (!setupMode) {
                    display.drawFloat(bpm);
                }
            }
            break;

        case Event::ENCODER_ROTATE:
            if (setupMode) {
                menuHandler(encoder.direction);
            } else {
                if (metronome.mode == Metronome::Mode::INTERNAL) {
                    float increment = encoderPressed ? 0.1 : 1;
                    if (encoder.direction == 1)
                    {
                        can_data += 10;
                        metronome.setBPM(metronome.getBPM() + increment);
                    }
                    else
                    {
                        can_data -= 10;
                        metronome.setBPM(metronome.getBPM() - increment);
                    }
                    display.drawFloat(metronome.getBPM());
                }
            }
            break;

        case Event::ENCODER_PRESS:
            if (setupMode) {
                menu.handleSelect();
                display.drawString(menu.getActiveItemText());
            }
            break;

        case Event::ENCODER_RELEASE:
            break;

        case Event::MIDI_RECEIVED:
        {
            // Copy the 3-byte MIDI message out of the shared buffer and let the MIDI parser fan it out to callbacks
            uint8_t msg[3] = {MIDI::BUFFER_IN[0], MIDI::BUFFER_IN[1], MIDI::BUFFER_IN[2]};
            midi.parseMessage(msg);
            break;
        }

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
    // if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
    // {
    //     OK_ERROR_HANDLER(HAL_ERROR, "Watchdog initialization failed");
    // }

    // AnalogIn::initialize(20000);
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

extern "C" void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4) {
        transport_ppqn24.write(0);
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
        ledReset.write(1);
        break;
    default:
        break;
    }
    return error;
}