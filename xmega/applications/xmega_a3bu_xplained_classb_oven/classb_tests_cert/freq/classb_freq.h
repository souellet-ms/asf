/**
 * \file
 *
 * \brief Settings and definitions for the CPU frequency test.
 *
 * \par Application note:
 *      AVR1610: Guide to IEC60730 Class B compliance with XMEGA
 *
 * Copyright (c) 2011-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

#ifndef CLASSB_FREQ_H_
#define CLASSB_FREQ_H_

#include <asf.h>
#include "classb_rtc_common.h"
#include "error_handler.h"

/**
 * \name Internal string manipulation macros
 * @{
 */

#define STRINGIZE_AUX(X) #X
#define STRINGIZE(X) STRINGIZE_AUX(X)
#define CONCAT3(X,Y,Z) X##Y##Z
#define LABEL(X,Y,Z) CONCAT3(X,Y,Z)

/** @} */

/**
 * \defgroup cpu_freq CPU Frequency Test
 *
 * \brief Self-diagnostic test for the CPU frequency.
 *
 * The test consists of monitoring the frequency of a Timer/Counter (TC) using
 * the Real Time Counter (RTC) module as a time reference. Note that the TC has
 * the same clock domain as the CPU and that the RTC module in XMEGA can have a
 * clock source independent from the CPU clock domain. Also, note that
 * \ref CLASSB_FREQ_TEST has to be defined. For more details see
 * \ref rtc_driver.
 *
 * The test is implemented as follows:
 * -# an RTC compare interrupt is scheduled periodically with
 * \ref classb_rtc_setup() and a TC is started with
 * \ref classb_freq_setup_timer().
 * -# the RTC interrupt calls \ref classb_freq_callback(), which compares the
 * absolute difference between the count in the TC and a predefined reference.
 * If it is greater than expected, the error handler
 * \ref CLASSB_ERROR_HANDLER_FREQ() is called.
 *
 * The TC overflow interrupt increases a counter that can be used as the most
 * significant word (bits 31 to 16) for the TC counter value. Further, there is
 * a configurable limit for the number of overflows. If this is exceeded, the
 * program assumes that the RTC is working correctly and the error handler
 * is called as well.
 *
 * The test is flexible and it is possible to choose some settings for the TC
 * and RTC. It is important to choose values for \ref CLASSB_RTC_INT_PERIOD,
 * \ref CLASSB_RTC_FREQ, \ref CLASSB_TC_PRESCALER and \ref CLASSB_TC_TOLERANCE
 * so that the frequency of the CPU \ref F_CPU is estimated reliably. Further,
 * there should not be an overflow in \ref CLASSB_TC_COUNT_REF. Suggested values
 * have been included in the documentation.
 *
 * @{
 */

/**
 * \defgroup cpu_freq_conf Settings
 *
 * \brief This should be configured by the user.
 *
 * @{
 */

#ifdef __DOXYGEN__
/**
 * \brief The system frequency.
 *
 * This is the expected frequency (Hz) of the CPU. This value has to be defined
 * at the compiler level and it is only used to compute the TC count reference,
 * i.e., not to set the system clock. Therefore, this value should be consistent
 * with the system clock settings in the main application. However, this
 * constant could be used to test the self-diagnostic routine, i.e., a wrong
 * value could be set to check whether the failure is detected.
 *
 * \note The system runs from the internal 2Mhz RC oscillator by default.
 */
#  define F_CPU <value>
#endif

/**
 * \brief TC module selection
 *
 * This is the specifier of the TC module that should be used for the CPU
 * frequency test, e.g., C0 -> TCC0.
 */
#define CLASSB_TC_MOD         D1

/**
 * \brief TC prescaler
 *
 * The TC runs on the system clock scaled down by this parameter.
 * Possible values are 1, 2, 4, 8, 64, 256 or 1024.
 */
#define CLASSB_TC_PRESCALER   64

/**
 * \brief Tolerance for the CPU frequency deviation
 *
 * This value defines the tolerance (an integer, %) for the deviation between
 * the measured and expected CPU frequency, e.g., 25 -> 25%.
 */
#define CLASSB_TC_TOLERANCE 25UL

/** @} */

/**
 * \defgroup cpu_freqz_int_conf Internal settings
 *
 * \brief These constants should not be modified by user.
 *
 * @{
 */

/** Label for the TC module */
#define CLASSB_TEST_TC          LABEL(TC, CLASSB_TC_MOD,)
/** Label for the overflow interrupt vector for the chosen TC */
#define CLASSB_TEST_TC_OVF_vect LABEL(TC, CLASSB_TC_MOD, _OVF_vect)
/** Label for the TC prescaler group configuration */
#define CLASSB_TC_PRESCALER_gc  LABEL(TC_CLKSEL_DIV, CLASSB_TC_PRESCALER, _gc)

/**
 * \brief TC period
 *
 * The 16-bit TC will generate an interrupt when the count reaches this value.
 * The code is built counting that this is the largest possible value, i.e.,
 * 0xFFFF.
 */
#define CLASSB_TC_PER       0xFFFF

/**
 * \brief TC frequency
 *
 * The frequency of the TC is F_CPU divided by the prescaling factor.
 */
#define CLASSB_TC_FREQ      (uint32_t)(F_CPU / CLASSB_TC_PRESCALER)

/**
 * \brief TC count reference
 *
 * This is the reference value for TC count within one RTC interrupt period.
 * Reference = F_tc * Interrupt_Period_rtc / F_rtc
 * This can overflow if \ref CLASSB_TC_FREQ and \ref CLASSB_RTC_INT_PERIOD are
 * too large.
 *
 * \note Multiplying by 1e0L forces the preprocessor to do operations with
 * higher precision.
 */
#define CLASSB_TC_COUNT_REF (uint32_t)(((1e0L * CLASSB_TC_FREQ) \
		* CLASSB_RTC_INT_PERIOD) / CLASSB_RTC_FREQ)

/**
 * \brief Maximum absolute difference between reference and estimated frequency
 *
 * \note Multiplying by 1e0L forces the preprocessor to do operations with
 * higher precision.
 */
#define CLASSB_TC_MAX_DIFF   (uint32_t)((1e0L * CLASSB_TC_COUNT_REF \
		* CLASSB_TC_TOLERANCE) / 100UL)

/**
 * \brief TC maximum number of overflows
 *
 * This is a limit for the number of TC overflows. If this is exceeded, the RTC
 * is assumed to be faulty and the error handler called.
 */
#define CLASSB_TC_MAX_OVF_COUNT (uint16_t)((CLASSB_TC_COUNT_REF \
		+ CLASSB_TC_MAX_DIFF) >> 16)

/** @} */

/**
 * \defgroup cpu_freq_func Functions
 *
 * \brief Setup and test functions
 *
 * @{
 */

void classb_freq_setup_timer(void);
void classb_freq_callback(void );
void classb_freq_tc_callback(void);

/** @} */

/** @} */

#endif /* CLASSB_FREQ_H_ */
