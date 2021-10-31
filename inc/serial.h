/*******************************************************************************
 * Copyright (c) 2021-2022, OKMCU Development Team 
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Website: http://www.okmcu.com
 *
 * File Description: Template description.
 *
 * Change Logs:
 * Date         Author       Notes    
 * 2021-10-30   Wentao SUN   first version
 * 
 ******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SERIAL_H
#define __SERIAL_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
/* Exported constants --------------------------------------------------------*/
#define COM_BYTESZ_5        5
#define COM_BYTESZ_6        6
#define COM_BYTESZ_7        7
#define COM_BYTESZ_8        8

#define COM_STOPBITS_1      0
#define COM_STOPBITS_1P5    1
#define COM_STOPBITS_2      2

#define COM_PARITY_NONE     0
#define COM_PARITY_ODD      1
#define COM_PARITY_EVEN     2
#define COM_PARITY_MARK     3
#define COM_PARITY_SPACE    4
/* Exported types ------------------------------------------------------------*/
typedef struct com_param_s {
    uint32_t baudrate;
    uint8_t bytesize;
    uint8_t parity;
    uint8_t stopbits;
} com_param_t;

typedef void *com_handle_t;
/* Exported macro ------------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Template brief.
  * @param  none
  * @retval none
  * @note   Template note.
  */
int32_t com_open(const char *port, com_param_t *param, com_handle_t *handle);

/**
  * @brief  Template brief.
  * @param  none
  * @retval none
  * @note   Template note.
  */
int32_t com_send(com_handle_t handle, const uint8_t *buf, size_t size);

/**
  * @brief  Template brief.
  * @param  none
  * @retval none
  * @note   Template note.
  */
int32_t com_recv(com_handle_t handle, uint8_t *buf, size_t size, size_t *rxcnt, uint32_t timeout);

/**
  * @brief  Template brief.
  * @param  none
  * @retval none
  * @note   Template note.
  */
int32_t com_close(com_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* __SERIAL_H */

/******************************** END OF FILE *********************************/

