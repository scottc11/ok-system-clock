#include "uart.h"

UART_HandleTypeDef huart1;

// Single-byte buffer used for interrupt-driven MIDI reception
static uint8_t midi_rx_byte;

void uart_init()
{
    HAL_StatusTypeDef status;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Peripheral clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    /* USART1 init */
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 31250;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    status = HAL_UART_Init(&huart1);
    if (status != HAL_OK)
    {
        OK_ERROR_HANDLER(status, "HAL_UART_Init");
    }

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, RTOS_ISR_DEFAULT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    // Prime the UART to start receiving MIDI data one byte at a time
    status = HAL_UART_Receive_IT(&huart1, &midi_rx_byte, 1);
    if (status != HAL_OK)
    {
        OK_ERROR_HANDLER(status, "HAL_UART_Receive_IT");
    }
}

/**
 * @brief UART data received callback
 *
 * @param huart
 */
extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        // Feed the incoming byte to the MIDI parser. When a full
        // 3-byte MIDI message has been assembled, signal the
        // sequencer task to handle it outside of the ISR.
        if (midi.processByte(midi_rx_byte))
        {
            dispatch_event_isr(Event::MIDI_RECEIVED);
        }

        // Re-arm reception of the next byte
        HAL_StatusTypeDef status = HAL_UART_Receive_IT(&huart1, &midi_rx_byte, 1);
        if (status != HAL_OK)
        {
            OK_ERROR_HANDLER(status, "HAL_UART_Receive_IT");
        }
    }
}

/**
 * @brief This function handles USART1 global interrupt.
 */
extern "C" void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}
