/*******************************************************************************
 * Copyright 2019-2021 Microchip FPGA Embedded Systems Solutions.
 *
 * SPDX-License-Identifier: MIT
 *
 * Implementation of uart_putstring/g().
 * This is function is intended to be used from ee_printf().
 */

#include <assert.h>
#include <sbi/riscv_asm.h>
#include <slice/slice_log.h>
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "harts.h"
#include "mss_uart.h"

//#include boards/xxx/fpga_design_config/clocks/hw_mss_clks.h
#include "clocks/hw_mss_clks.h"
//#include "uart_helper.h"
#define UART_HELPER_MAX_GETLINE 100u
#define TICKS_PER_SEC ((unsigned long long)LIBERO_SETTING_MSS_RTC_TOGGLE_CLK)

uint64_t get_time(void) { return csr_read(CSR_MCYCLE); }

bool timer_is_elapsed(uint64_t startTick, uint64_t durationInTicks) {
  return (get_time() > (startTick + durationInTicks));
}

unsigned long hss_start_time[5] = {0};

static inline mss_uart_instance_t *get_uart_instance(int hartid) {
  mss_uart_instance_t *pUart;

  switch (hartid) {
    default:
      pUart = &g_mss_uart0_lo;
      break;
    case HSS_HART_E51:
      pUart = &g_mss_uart0_lo;
      break;
    case HSS_HART_U54_1:
    case HSS_HART_U54_2:
      pUart = &g_mss_uart2_lo;
      break;
    case HSS_HART_U54_3:
    case HSS_HART_U54_4:
      pUart = &g_mss_uart1_lo;
      break;
  }

  return pUart;
}

int uart_putstring(int hartid, char *p) {
  const uint32_t len = (uint32_t)strlen(p);

  mss_uart_instance_t *pUart = get_uart_instance(hartid);
  MSS_UART_polled_tx_string(pUart, (const uint8_t *)p);
  // TODO: if hartId is zero (i.e., E51), replace this with non-blocking
  // queue implementation, with HSS_UART state machine consuming from queues...
  return len;
}

void uart_putc(int hartid, const char ch) {
  uint8_t string[2];
  string[0] = (uint8_t)ch;
  string[1] = 0u;

  mss_uart_instance_t *pUart = get_uart_instance(hartid);
  MSS_UART_polled_tx_string(pUart, (const uint8_t *)string);
}

ssize_t uart_getline(char **pBuffer, size_t *pBufLen) {
  ssize_t result = 0;
  bool finished = false;
  static char
      myBuffer[UART_HELPER_MAX_GETLINE];  // static to be stack friendly
  const size_t bufferLen = array_size(myBuffer);

  memset(myBuffer, 0, bufferLen);

  uint8_t cBuf[1];
  while (!finished) {
    while (0 == MSS_UART_get_rx(&g_mss_uart0_lo, cBuf, 1))
      ;

    switch (cBuf[0]) {
      case '\r':
        MSS_UART_polled_tx(&g_mss_uart0_lo, cBuf, 1u);
        finished = true;
        break;

      case '\n':
        MSS_UART_polled_tx(&g_mss_uart0_lo, cBuf, 1u);
        finished = true;
        break;

      case 0x7Fu:  // delete
        if (result) {
          result--;
          MSS_UART_polled_tx(&g_mss_uart0_lo, (uint8_t const *)"\033[D \033[D",
                             7u);
          myBuffer[result] = 0;
        }
        break;

      case 0x08u:  // backspace - ^H
        if (result) {
          result--;
          MSS_UART_polled_tx(&g_mss_uart0_lo, (uint8_t const *)" \033[D", 4u);
          myBuffer[result] = 0;
        }
        break;

      case 0x03u:  // intr - ^C
        result = -1;
        myBuffer[0] = 0;
        finished = true;
        break;

      case 0x1Bu:  // ESC
        result = -1;
        myBuffer[0] = 0;
        finished = true;
        break;

      case 0x04u:  // ^D
        if (result == 0) {
          result = -1;
          myBuffer[0] = 0;
          finished = true;
        }
        break;

      default:
        if (result < bufferLen) {
          MSS_UART_polled_tx(&g_mss_uart0_lo, cBuf, 1u);
          myBuffer[result] = cBuf[0];
          result++;
        }
        break;
    }
  }

  const char crlf[] = "\n";
  MSS_UART_polled_tx_string(&g_mss_uart0_lo, (const uint8_t *)crlf);

  if (result > 0) {
    *pBuffer = myBuffer;
    *pBufLen = (size_t)result;
  } else {
    *pBuffer = NULL;
    *pBufLen = 0u;
  }

  return result;
}

bool uart_getchar(uint8_t *pbuf, int32_t timeout_sec, bool do_sec_tick) {
  bool result = false;
  bool done = false;
  uint8_t rx_buff[1];
  uint64_t start_time = 0u;
  uint64_t last_sec_time = 0u;

  // if (timeout_sec > 0) {
  start_time = last_sec_time = get_time();
  //}

  const uint64_t timeout_ticks = timeout_sec * TICKS_PER_SEC;
  //(void)MSS_UART_get_rx_status(&g_mss_uart0_lo); // clear sticky status

  while (!done) {
    size_t received = MSS_UART_get_rx(&g_mss_uart0_lo, rx_buff, 1u);
    if (0u != received) {
      done = true;
      if (MSS_UART_NO_ERROR == MSS_UART_get_rx_status(&g_mss_uart0_lo)) {
        *pbuf = rx_buff[0];
        result = true;
        break;
      } else {
        slice_log_printf(SLICE_LOG_ERR, "UART error");
      }
    }

    if (do_sec_tick && timer_is_elapsed(last_sec_time, TICKS_PER_SEC)) {
      const uint8_t dot = '.';
      MSS_UART_polled_tx(&g_mss_uart0_lo, &dot, 1);
      last_sec_time = get_time();
    }

    if (timeout_sec < 0) {
      ;  // blocking until UART data received, so nothing extra to do here...
    } else if (timeout_sec > 0) {
      // time limited
      done = timer_is_elapsed(start_time, timeout_ticks);
    } else /* timeout == 0 */ {
      // one-shot
      break;
    }
  }

  return result;
}

bool HSS_UARTInit(void) {
  hss_start_time[0] = csr_read(CSR_MCYCLE);
  MSS_UART_init(
      &g_mss_uart0_lo, MSS_UART_115200_BAUD,
      MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);

  // default all UARTs to 115200 for now
  // subsequent OS loads can change these if needed...
  MSS_UART_init(
      &g_mss_uart1_lo, MSS_UART_115200_BAUD,
      MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);

  MSS_UART_init(
      &g_mss_uart2_lo, MSS_UART_115200_BAUD,
      MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);

  MSS_UART_init(
      &g_mss_uart3_lo, MSS_UART_115200_BAUD,
      MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);

  MSS_UART_init(
      &g_mss_uart4_lo, MSS_UART_115200_BAUD,
      MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);
  return true;
}