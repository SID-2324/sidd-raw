#include "DMA_HANDLER.h"

volatile uint8_t base_tx_ready = 1;
volatile uint8_t wrist_tx_ready = 1;
extern volatile uint8_t delay_finished_flag;

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    // CRITICAL FIX: Only flip the pin back to receive when the DMA is entirely finished!
    HAL_HalfDuplex_EnableReceiver(huart);

    // Unlock the respective pipelines so the next command can queue up
    if (huart->Instance == USART1)
    {
        base_tx_ready = 1;
    }
    else if (huart->Instance == USART2)
    {
        wrist_tx_ready = 1;
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        delay_finished_flag = 1;
        HAL_TIM_Base_Stop_IT(htim);
    }
}
