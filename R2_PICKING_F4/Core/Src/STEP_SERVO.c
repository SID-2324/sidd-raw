#include "STEP_SERVO.h"
#include "STS_CTRL.h"
#include <stdio.h>
#include <string.h>
#include "steprrr.h"
#include "VL53L1X_api.h"
#include "vl53l1_platform.h"
#include "usbd_cdc_if.h" // Native USB Transmit
#include "PICKING_AUTONOMOUS.h"
#include "TIMER_DELAY.h"

static uint8_t last_jog_rx_data = 255;
static uint32_t last_chunk_time = 0;

// --- SENSOR ADDRESSES ---
#define S1_ADDR 0x54 // Horizontal Reach
#define S2_ADDR 0x56 // Vertical Drop
#define S3_ADDR 0x58 // Pneumatic Confirm
#define PNEUMATIC_PORT GPIOA
#define PNEUMATIC_PIN GPIO_PIN_5

// --- THE MEMORY BANK ---
static int target_horizontal_steps = 0;
static int target_vertical_steps = 0;
static uint16_t target_pneumatic_trigger_mm = 80; // Fixed trigger zone
static uint8_t stacked_box_count = 0;

// Flag to ensure we don't move before we scan
static uint8_t calibration_complete = 0;

static void Safe_CDC_Transmit(uint8_t *Buf, uint16_t Len)
{
    uint32_t start = HAL_GetTick();
    uint8_t res;

    do {
        res = CDC_Transmit_FS(Buf, Len);
        if (res == USBD_OK) {
            break;
        }
        HAL_Delay(1);
    } while (res == USBD_BUSY && (HAL_GetTick() - start) < 50);
}

// =================================================================
// MANUAL MODE: JOGGING (Now silent to avoid spamming USB)
// =================================================================
void STS_JogCommand(UART_HandleTypeDef *huart_base, UART_HandleTypeDef *huart_wrist, uint8_t rx_data)
{
    uint16_t chunk_distance = 100;
    uint16_t chunk_distance_1 = 500;

    uint16_t chunk_forward  = chunk_distance;
    uint16_t chunk_backward = chunk_distance | 0x8000;
    uint16_t chunk_forward_1  = chunk_distance_1;
    uint16_t chunk_backward_1 = chunk_distance_1 | 0x8000;

    uint16_t speed = 4900;
    //uint32_t chunk_delay = 38;


    if (rx_data != last_jog_rx_data) {
        if (rx_data == '0') {
            STS_WritePosition(huart_base, 120, 0, 0);
            HAL_Delay(1.5);
            STS_WritePosition(huart_base, 121, 0, 0);
            HAL_Delay(1.5);
            STS_WritePosition(huart_base, 122, 0, 0);
            HAL_Delay(1.5);
            STS_WritePosition(huart_base, 123, 0, 0);
            HAL_Delay(1.5);
            STS_WritePosition(huart_wrist, 124, 0, 0);
            HAL_Delay(1.5);
            STS_WritePosition(huart_wrist, 125, 0, 0);
            HAL_Delay(1.5);
            STS_WritePosition(huart_wrist, 126, 0, 0);
            HAL_Delay(1.5);
            STS_WritePosition(huart_wrist, 127, 0, 0);
        }
        last_jog_rx_data = rx_data;
    }

    if (rx_data != '0') {
        //if (HAL_GetTick() - last_chunk_time >= chunk_delay)
        {


        	switch(rx_data) {
            case 'a':
                 STS_WritePosition(huart_base, 107, chunk_forward, speed);
                 Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());
                 STS_WritePosition(huart_base, 1, chunk_backward, speed);
                 char msg_a[] = "CASE-A\n";
                 Safe_CDC_Transmit((uint8_t*)msg_a, strlen(msg_a));

                 break;
            case 2:
                 STS_WritePosition(huart_base, 107, chunk_backward, speed);
                 Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());
                 STS_WritePosition(huart_base, 1, chunk_forward, speed);
                 break;
            case 3:
                 STS_WritePosition(huart_base, 4, chunk_forward, speed);
                 Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());
                 STS_WritePosition(huart_base, 2, chunk_backward, speed);
                 break;
            case 4:
                 STS_WritePosition(huart_base, 4, chunk_backward, speed);
                 Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());
                 STS_WritePosition(huart_base, 2, chunk_forward, speed);
                 break;
            case 5:
            	STS_WritePosition(huart_base, 103, chunk_forward_1, speed);
            	break;
            case 6:
//                	STS_WritePosition(huart_base, 120, chunk_backward_1, speed);
//                	STS_WritePosition(huart_wrist, 124, chunk_forward_1, speed);
//
//                	STS_WritePosition(huart_base, 121, chunk_forward_1, speed);
//                	STS_WritePosition(huart_wrist, 125, chunk_backward_1, speed);
//
//                	STS_WritePosition(huart_base, 122, chunk_backward_1, speed);
//                	STS_WritePosition(huart_wrist, 126, chunk_forward_1, speed);
//
//                	STS_WritePosition(huart_base, 123, chunk_forward_1, speed);
//                	STS_WritePosition(huart_wrist, 127, chunk_backward_1, speed);
//                	                	break;

            {
                                // Blast Pair 1 (Base + Wrist simultaneously via DMA)
                                STS_WritePosition(huart_base, 120, chunk_backward_1, speed);
                                STS_WritePosition(huart_wrist, 124, chunk_forward_1, speed);
                                Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());

                                // Blast Pair 2
                                STS_WritePosition(huart_base, 121, chunk_forward_1, speed);
                                STS_WritePosition(huart_wrist, 125, chunk_backward_1, speed);
                                Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());

                                // Blast Pair 3
                                STS_WritePosition(huart_base, 122, chunk_backward_1, speed);
                                STS_WritePosition(huart_wrist, 126, chunk_forward_1, speed);
                                Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());

                                // Blast Pair 4
                                STS_WritePosition(huart_base, 123, chunk_forward_1, speed);
                                STS_WritePosition(huart_wrist, 127, chunk_backward_1, speed);
                                break;
                            }

            case 7:
            	STS_WritePosition(huart_base, 120, chunk_forward_1, speed);
            	Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());
            	STS_WritePosition(huart_base, 121, chunk_backward_1, speed);
            	Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());
            	STS_WritePosition(huart_wrist, 124, chunk_backward_1, speed);
            	Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());
            	STS_WritePosition(huart_wrist, 125, chunk_forward_1, speed);
            	break;

            case 8:
            	STS_WritePosition(huart_base, 120, chunk_backward_1, speed);
            	Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());
            	STS_WritePosition(huart_base, 121, chunk_forward_1, speed);
            	Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());
            	STS_WritePosition(huart_wrist, 124, chunk_forward_1, speed);
            	Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());;
            	STS_WritePosition(huart_wrist, 125, chunk_backward_1, speed);
            	break;


            case 9:
                STS_WritePosition(huart_base, 122, chunk_backward_1, speed);
                Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());
                STS_WritePosition(huart_base, 123, chunk_forward_1, speed);
                Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());
                STS_WritePosition(huart_wrist, 126, chunk_forward_1, speed);
                Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());
                STS_WritePosition(huart_wrist, 127, chunk_backward_1, speed);
                break;

            case 10:
                STS_WritePosition(huart_base, 122, chunk_forward_1, speed);
                Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());
                STS_WritePosition(huart_base, 123, chunk_backward_1, speed);
                Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());
                STS_WritePosition(huart_wrist, 126, chunk_backward_1, speed);
                Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());
                STS_WritePosition(huart_wrist, 127, chunk_forward_1, speed);
                break;


            case 11:
//                	STS_WritePosition(huart_base, 120, chunk_forward_1, speed);
//                	STS_WritePosition(huart_wrist, 124, chunk_backward_1, speed);
//
//                	STS_WritePosition(huart_base, 121, chunk_backward_1, speed);
//                	STS_WritePosition(huart_wrist, 125, chunk_forward_1, speed);
//
//                	STS_WritePosition(huart_base, 122, chunk_forward_1, speed);
//                	STS_WritePosition(huart_wrist, 126, chunk_backward_1, speed);
//
//                	STS_WritePosition(huart_base, 123, chunk_backward, speed);
//                	STS_WritePosition(huart_wrist, 127, chunk_forward_1, speed);

            {
                                // Blast Pair 1 (Base + Wrist simultaneously via DMA)
                                STS_WritePosition(huart_base, 120, chunk_forward_1, speed);
                                STS_WritePosition(huart_wrist, 124, chunk_backward_1, speed);
                                Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());

                                // Blast Pair 2
                                STS_WritePosition(huart_base, 121, chunk_backward_1, speed);
                                STS_WritePosition(huart_wrist, 125, chunk_forward_1, speed);
                                Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());

                                // Blast Pair 3
                                STS_WritePosition(huart_base, 122, chunk_forward_1, speed);
                                STS_WritePosition(huart_wrist, 126, chunk_backward_1, speed);
                                Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());

                                // Blast Pair 4
                                STS_WritePosition(huart_base, 123, chunk_backward_1, speed);
                                STS_WritePosition(huart_wrist, 127, chunk_forward_1, speed);
                                break;
                            }



            case 12:
            	STS_WritePosition(huart_base, 5, chunk_backward_1, 4900);
            	Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());
            	STS_WritePosition(huart_base, 101, chunk_forward_1, 4900);
            	break;
            case 13:
            	printf("System Initialized! MOVING STEPPER.\r\n");
            	//Stepper_JogContinuous(50, 0, 50);
            	break;
            case 14:
            	printf("System Initialized! MOVING STEPPER.\r\n");
            	//Stepper_JogContinuous(50, 1, 50);
            	break;         	                        }
            last_chunk_time = HAL_GetTick();
        }
    }
}

// =================================================================
// AUTO MODE: AUTONOMOUS KINEMATICS
// =================================================================
void STS_AutoPickCommand(UART_HandleTypeDef *huart_base, UART_HandleTypeDef *huart_wrist, uint8_t ps4_button)
{
    char usb_buffer[128]; // Temporary buffer to build messages

    if (calibration_complete == 0) {
        char err[] = "[STM32 ERROR]: Not calibrated! Switch out and back into Auto Mode.\r\n";
        CDC_Transmit_FS((uint8_t*)err, strlen(err));
        return;
    }

    int retract_h_steps = target_horizontal_steps | 0x8000;
    int retract_v_steps = target_vertical_steps | 0x8000;
    uint16_t live_pad_dist = 0;

    switch(ps4_button) {

        // -------------------------------------------------------------
        // BUTTON 1: LOAD BOX 1 (Base Plate)
        // -------------------------------------------------------------
        case 1:
            if (stacked_box_count > 0) {
                char err_msg[] = "[STM32 ERROR]: Box 1 already loaded!\r\n";
                CDC_Transmit_FS((uint8_t*)err_msg, strlen(err_msg));
                break;
            }

            char start_msg[] = "[STM32 ACK]: EXECUTING LOAD: BOX 1\r\n";
            CDC_Transmit_FS((uint8_t*)start_msg, strlen(start_msg));
            HAL_Delay(5);

            // 1. EXTEND HORIZONTAL using calibrated math
            HAL_Delay(2);
            STS_WritePosition(huart_base, 1, target_horizontal_steps, 1500);
            HAL_Delay(2);
            STS_WritePosition(huart_base, 4, target_vertical_steps, 1500);
            HAL_Delay(10000);

            // 3. FINAL CONFIRMATION & GRAB (Using S3 live read)
            live_pad_dist = VL53L1X_GetStableAverage(S3_ADDR, 2);
            if (live_pad_dist > 5 && live_pad_dist <= target_pneumatic_trigger_mm) {
                HAL_GPIO_WritePin(PNEUMATIC_PORT, PNEUMATIC_PIN, GPIO_PIN_SET); // Grip
                HAL_Delay(500);
            } else {
                char abort_msg[] = "[STM32 ERROR]: Box not in pads! Aborting.\r\n";
                CDC_Transmit_FS((uint8_t*)abort_msg, strlen(abort_msg));
                break;
            }

            // 4. LIFT VERTICAL
            STS_WritePosition(huart_base, 4, retract_v_steps, 1500);
            HAL_Delay(2);
            STS_WritePosition(huart_base, 1, retract_h_steps, 1500);
            HAL_Delay(2);

            STS_WritePosition(huart_base, 2, 2000, 1500);
            HAL_Delay(2000);


            stacked_box_count = 1;

            sprintf(usb_buffer, "[STM32 DONE]: Box 1 Load Complete using %d H-Steps.\r\n", target_horizontal_steps);
            CDC_Transmit_FS((uint8_t*)usb_buffer, strlen(usb_buffer));
            break;

        // -------------------------------------------------------------
        // BUTTON 2: LOAD BOX 2 (Tier 2)
        // -------------------------------------------------------------
        case 2:
            if (stacked_box_count != 1) {
                char err_msg[] = "[STM32 ERROR]: Must load Box 1 first!\r\n";
                Macro_PickSpearhead(huart_base, huart_wrist);
                CDC_Transmit_FS((uint8_t*)err_msg, strlen(err_msg));
                break;
            }

            char start_msg2[] = "[STM32 ACK]: EXECUTING LOAD: BOX 2\r\n";
            CDC_Transmit_FS((uint8_t*)start_msg2, strlen(start_msg2));
            HAL_Delay(5);

            // (Your Kinematics 1-5 here)

            stacked_box_count = 2;

            char done_msg2[] = "[STM32 DONE]: Box 2 Load Complete.\r\n";
            CDC_Transmit_FS((uint8_t*)done_msg2, strlen(done_msg2));
            break;

        // -------------------------------------------------------------
        // BUTTON 3: UNLOAD BOX 2
        // -------------------------------------------------------------
        case 3:
            char start_msg3[] = "[STM32 ACK]: EXECUTING UNLOAD: BOX 2\r\n";
            CDC_Transmit_FS((uint8_t*)start_msg3, strlen(start_msg3));
            HAL_Delay(5);

            // Unload kinematics here

            stacked_box_count = 1;
            char done_msg3[] = "[STM32 DONE]: Box 2 Unload Complete.\r\n";
            CDC_Transmit_FS((uint8_t*)done_msg3, strlen(done_msg3));
            break;

        // -------------------------------------------------------------
        // BUTTON 4: UNLOAD BOX 1
        // -------------------------------------------------------------
        case 4:
            char start_msg4[] = "[STM32 ACK]: EXECUTING UNLOAD: BOX 1\r\n";
            CDC_Transmit_FS((uint8_t*)start_msg4, strlen(start_msg4));
            HAL_Delay(5);

            // Unload kinematics here

            stacked_box_count = 0;
            char done_msg4[] = "[STM32 DONE]: Box 1 Unload Complete.\r\n";
            CDC_Transmit_FS((uint8_t*)done_msg4, strlen(done_msg4));
            break;
    }
}

// =================================================================
// 3D ENVIRONMENT CALIBRATION (Fires automatically when 'm' is pressed)
// =================================================================
void STS_CalibrateEnvironment(void)
{
    char usb_buffer[128]; // Buffer for USB strings

    char init_msg[] = "\r\n=== INITIATING 3D SENSOR SCAN ===\r\n";
    CDC_Transmit_FS((uint8_t*)init_msg, strlen(init_msg));
    HAL_Delay(5);

    // 1. Get Stable Averages
    uint16_t raw_h_dist = VL53L1X_GetStableAverage(S1_ADDR, 5);
    uint16_t raw_v_dist = VL53L1X_GetStableAverage(S2_ADDR, 5);
    uint16_t raw_p_dist = VL53L1X_GetStableAverage(S3_ADDR, 5);

    sprintf(usb_buffer, "[STM32 SCAN]: H: %dmm | V: %dmm | P: %dmm\r\n", raw_h_dist, raw_v_dist, raw_p_dist);
    CDC_Transmit_FS((uint8_t*)usb_buffer, strlen(usb_buffer));
    HAL_Delay(5);

    // CRITICAL: Check if sensors are failing
    if (raw_h_dist == 0 && raw_v_dist == 0) {
        char err[] = "[STM32 FATAL]: Sensors reporting 0mm. Check I2C Bus!\r\n";
        CDC_Transmit_FS((uint8_t*)err, strlen(err));
        calibration_complete = 0;
        return;
    }

    // 2. Calculate Horizontal Target Steps
    uint16_t safe_h_mm = (raw_h_dist > 270) ? (raw_h_dist - 270) : 0;
    target_horizontal_steps = (int)(((float)safe_h_mm / 157.0f) * 4096.0f);
    if (target_horizontal_steps > 3000) target_horizontal_steps = 3000;

    // 3. VERTICAL LOGIC
    if (raw_v_dist < 580 || raw_v_dist > 620) {
        int error_mm = 600 - (int)raw_v_dist;
        target_vertical_steps = (int)(((float)error_mm / 157.0f) * 4096.0f);

        if (target_vertical_steps > 2500) target_vertical_steps = 2500;
        if (target_vertical_steps < -2500) target_vertical_steps = -2500;
    } else {
        target_vertical_steps = 0;
    }

    // 4. Set Pneumatic Trigger Point
    target_pneumatic_trigger_mm = 80;
    calibration_complete = 1;

    sprintf(usb_buffer, "[STM32 SUCCESS]: CALIBRATION SAVED. H-Steps: %d | V-Steps: %d\r\n",
            target_horizontal_steps, target_vertical_steps);
    CDC_Transmit_FS((uint8_t*)usb_buffer, strlen(usb_buffer));
}

void SERVO_BOOT(UART_HandleTypeDef *huart_base, UART_HandleTypeDef *huart_wrist)
{
	// Blast Pair 1 (Base + Wrist simultaneously via DMA)
	                                    STS_WritePosition(huart_base, 120, 523 | 0x8000, 3000);
	                                    STS_WritePosition(huart_wrist, 124, 523, 3000);
	                                    Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());

	                                    // Blast Pair 2
	                                    STS_WritePosition(huart_base, 121, 523, 3000);
	                                    STS_WritePosition(huart_wrist, 125, 523 | 0x8000, 3000);
	                                    Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());

	                                    // Blast Pair 3
	                                    STS_WritePosition(huart_base, 122, 523 | 0x8000, 3000);
	                                    STS_WritePosition(huart_wrist, 126, 523, 3000);
	                                    Start_NonBlocking_Delay_us(1200); while(!Is_Delay_Finished());

	                                    // Blast Pair 4
	                                    STS_WritePosition(huart_base, 123, 523, 3000);
	                                    STS_WritePosition(huart_wrist, 127, 523 | 0x8000, 3000);


}
