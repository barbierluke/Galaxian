/*
 * timerA.c
 *
 *  Created on: Sep 15, 2017
 *      Author: luke
 */

#include "msp.h"
#include "Headers/timerA.h"
#include <limits.h>                 // for SHRT_MAX = 32767
#include <math.h>                   // modf()

void initTimerA()
{
    timerAs_in_use = 0;
    timerA0_counter = 0;
    timerA1_counter = 0;
    timerA2_counter = 0;
}

// a better interface would take the responsibility of managing which timers are in use out of the user's hands
TIMER_A_CONFIG configTimerA(TIMER_A timerA, uint16_t milli_time, void(*handler)(void))
{
    if (timerAs_in_use > 7)
        return TIMER_A_CONFIG_ERR_NO_INIT;

    switch(timerA)
    {
    case TIMERA_0:
        /* If in use, use reconfigureTimerA() instead */
        if (timerAs_in_use & TIMERA_0)
            return TIMER_A_CONFIG_ERR_IN_USE;
        else
            timerAs_in_use |= TIMERA_0;

        TA0CTL |= TACLR;                    // reset
#ifdef CALC_TIMER_CONFIG_FAST
        // calculateTimerConfigFast()
#elif CALC_TIMER_CONFIG_ONE_S
        TA0EX0 |= TAIDEX_0;
        TA0CTL |= TASSEL_1;
        TA0CTL |= ID_0;
        TA0CTL |= MC_1;
        TA0CTL |= TAIE;
        TA0CTL &= ~TAIFG;
        TA0R &= 0;
        TA0CCTL0 |= CCIS_0;                 // verify this
        TA0CCTL0 &= ~CAP;
        // interrupt here?
        TA0CCR0 |= 0xffff
#else
        // calculateTimerConfig()
        TIMER_A_CLK clk = TIMER_A_CLK_ACLK;
        uint8_t prescl_1 = 1;
        uint8_t prescl_2 = 1;
        uint16_t count = 32767;
#endif

        break;
    case TIMERA_1:
        /* If in use, use reconfigureTimerA() instead */
        if (timerAs_in_use & TIMERA_1)
            return TIMER_A_CONFIG_ERR_IN_USE;
        else
            timerAs_in_use |= TIMERA_1;

        break;
    case TIMERA_2:
        /* If in use, use reconfigureTimerA() instead */
        if (timerAs_in_use & TIMERA_2)
            return TIMER_A_CONFIG_ERR_IN_USE;
        else
            timerAs_in_use |= TIMERA_2;

        break;
    default:
        return TIMER_A_CONFIG_ERR_BAD_INPUT;
    }
    return TIMER_A_CONFIG_NO_ERROR;
}

void _configTimerA_Clk(TIMER_A timerA, TIMER_A_CLK clk)
{
    switch(timerA)
    {
    case TIMERA_0:
        if(clk == TIMER_A_CLK_SMCLK)
            TA0CTL |= TASSEL_1;
        else
            TA0CTL |= TASSEL_2;
        break;
    case TIMERA_1:
        if(clk == TIMER_A_CLK_SMCLK)
            TA1CTL |= TASSEL_1;
        else
            TA1CTL |= TASSEL_2;
        break;
    case TIMERA_2:
        if(clk == TIMER_A_CLK_SMCLK)
            TA2CTL |= TASSEL_1;
        else
            TA2CTL |= TASSEL_2;
        break;
    }
}

void _configTimer_AID(TIMER_A timerA, uint8_t id)
{
    uint8_t shift;
    switch(timerA)
    {
    case TIMERA_0:
        shift = id >> 1;
        shift = id >> 7 ? 0b11 : shift; // still working on this
        shift = shift << 6;
        break;
    case TIMERA_1:
        if(clk == TIMER_A_CLK_SMCLK)
            TA1CTL |= TASSEL_1;
        else
            TA1CTL |= TASSEL_2;
        break;
    case TIMERA_2:
        if(clk == TIMER_A_CLK_SMCLK)
            TA2CTL |= TASSEL_1;
        else
            TA2CTL |= TASSEL_2;
        break;
    }
}

void _configTimerA_TAIDEX(TIMER_A timerA, uint8_t taidex)
{

}

#ifdef CALC_TIMER_CONFIG_FAST
TIMER_A_CALC _calculateTimerConfigFast(TIMER_A_CLK * clk, uint8_t * prescl_1, uint8_t * prescl_2, uint16_t * count, uint16_t milli_time)
{

}
#endif


// TODO extend function capability to return error
#ifndef CALC_TIMER_CONFIG_FAST
TIMER_A_CALC _calculateTimerConfig(TIMER_A_CLK * clk, uint8_t * prescl_1, uint8_t * prescl_2, uint16_t * count, uint16_t milli_time)
{
    double seconds_time = milli_time / 1000;

    double clocks [] = {TIMER_A_CLK_SMCLK, TIMER_A_CLK_ACLK};
    uint8_t pre_1 [] = {1, 2, 4, 8};
    uint8_t pre_2 [] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint8_t best_config_indices [] = {0,0,0};
    double best_decimal = 0.999;
    uint16_t best_count = 0;

    uint8_t i, j, k;                                    // iterators
    double produced_count, decimal_part, integer_part;
    // TODO calculate size of arrays in more professional way
//   TODO move operations to RAM during runtime
    for(i = 0; i < 2; i++)                              // loop through possible clocks
    {
        for(j = 0; j < 4; j++)                          // loop through pre_1
        {
            for(k = 0; k < 8; k++)                      // loop through pre_2
            {
                produced_count = (seconds_time * pre_1[j] * pre_2[k]) / clocks[i];
                if(produced_count <= SHRT_MAX)
                {
                    decimal_part = modf(produced_count, &integer_part);
                    if (decimal_part == 0) // perfect solution
                    {
                        *clk = clocks[i];
                        *prescl_1 = pre_1[j];
                        *prescl_2 = pre_2[k];
                        *count = (unsigned short) integer_part;
                        return TIMER_A_CALC_EXACT_TIME;
                    }
                    else if (decimal_part < best_decimal)
                    {
                        best_config_indices[0] = i;
                        best_config_indices[1] = j;
                        best_config_indices[2] = k;
                        best_decimal = decimal_part;
                        best_count = (unsigned short) integer_part;
                    }
                }
                else                                    // next calc_count values will be even larger so skip
                {
                    break;
                }
            } // end k loop
        } // end j loop
    } // end i loop
    *clk = clocks[best_config_indices[0]];
    *prescl_1 = pre_1[best_config_indices[1]];
    *prescl_2 = pre_2[best_config_indices[2]];
    *count = best_count;
    return TIMER_A_CALC_ALMOST_TIME;
}
#endif

TIMER_A_START startTimerA(TIMER_A timerA)
{
    switch(timerA)
    {
    case TIMERA_0:
        if (!(timerAs_in_use & TIMERA_0))       // check if not configured
            return TIMER_A_START_ERR_NO_CONFIG;
        TA0R |= timerA0_counter;
        break;
    case TIMERA_1:
        if (!(timerAs_in_use & TIMERA_1))
            return TIMER_A_START_ERR_NO_CONFIG;
        TA1R |= timerA1_counter;
        break;
    case TIMERA_2:
        if (!(timerAs_in_use & TIMERA_2))
            return TIMER_A_START_ERR_NO_CONFIG;
        TA2R |= timerA2_counter;
        break;
    default:
        return TIMER_A_START_ERR_BAD_INPUT;
    }
    return TIMER_A_START_NO_ERROR;
}

void TimerAHandler(void)
{
    if(TA0CTL & TAIFG)
    {
        (*timerA0_handler)();
        TA0CTL &= ~TAIFG;
    }
    else if (TA1CTL & TAIFG)
    {
        (*timerA1_handler)();
        TA1CTL &= ~TAIFG;
    }
    else
    {
        (*timerA2_handler)();
        TA2CTL &= ~TAIFG;
    }

}
