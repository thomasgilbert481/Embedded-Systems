# Safety Audit Report — Project 7: Circle Following Robot
**Target:** MSP430FR2355
**Date:** March 2026
**Author:** Thomas Gilbert
**Auditor:** Automated review via Claude Code

---

## Summary

All 21 safety checks **PASS**. One low-risk observation is noted.

---

## 1. DAC Motor Power Safety

| # | Check | Result | Location |
|---|-------|--------|----------|
| 1a | `Init_REF()` called before `enable_interrupts()` | **PASS** | main.c before Init_Ports() |
| 1b | `Init_DAC()` called before any motor enable | **PASS** | main.c after Init_ADC() |
| 1c | `DAC_ENB` LOW at startup (dac.c), HIGH before Forward_On() (P7_WAIT_START exit), LOW at P7_DONE and SW2 emergency stop | **PASS** | dac.c:138, main.c P7_WAIT_START, main.c P7_DONE, interrupt_ports.c SW2 ISR |

**Notes:**
- `Init_REF()` contains a blocking busy-wait on `REFGENRDY`. Placing it before `Init_Conditions()` (which calls `enable_interrupts()`) ensures the wait cannot be preempted.
- `Init_DAC()` drives `DAC_ENB` LOW on exit, overriding the HIGH value set by `Init_Port2()`. This is intentional per the DAC safety design.

---

## 2. H-Bridge Safety

| # | Check | Result | Location |
|---|-------|--------|----------|
| 2a | No simultaneous forward + reverse on same motor | **PASS** | wheels.c all functions |
| 2b | `P7_FOLLOW_LINE` clears reverse before setting forward | **PASS** | main.c P7_FOLLOW_LINE (lines 523-524) |
| 2c | `Wheels_All_Off()` called at every state transition | **PASS** | main.c P7_FORWARD, P7_TURNING, P7_FOLLOW_LINE, P7_EXIT_TURN, P7_DONE |

**Notes:**
- `Forward_On()` and `Reverse_On()` clear the opposite-direction CCRs before writing their own.
- `Spin_CW_On()` and `Spin_CCW_On()` call `Wheels_All_Off()` first (all-zero before reprogramming).
- `P7_FOLLOW_LINE` directly writes TB3CCRx registers but always zeroes `LEFT_REVERSE_SPEED` and `RIGHT_REVERSE_SPEED` at the top of each pass through the state before setting any forward speed.

---

## 3. ADC Safety

| # | Check | Result | Location |
|---|-------|--------|----------|
| 3a | IR emitter OFF before ambient sample, ON before white/black samples | **PASS** | main.c CAL_AMBIENT, CAL_WHITE |
| 3b | `threshold_left/right` computed in CAL_BLACK before use in P7_FORWARD | **PASS** | main.c CAL_BLACK (→ P7_WAIT_START → P7_FORWARD) |
| 3c | No divide-by-zero or unsigned underflow in `(white + black) / 2` | **PASS** | main.c CAL_BLACK |

---

## 4. Interrupt Safety

| # | Check | Result | Location |
|---|-------|--------|----------|
| 4a | CCR0 never disabled during active driving states | **PASS** | interrupts_timers.c; no `TB0CCTL0 &= ~CCIE` present |
| 4b | All ISR-shared variables declared `volatile` | **PASS** | main.c: p7_state, p7_timer, p7_running, elapsed_tenths, p7_timer_running, sw1/sw2_debounce_count |
| 4c | All cross-module externs match definitions | **PASS** | interrupt_ports.c, interrupts_timers.c |

**Notes:**
- Previous project versions disabled CCR0 during SW press debounce; this was removed because CCR0 drives `p7_timer` and `elapsed_tenths` which must run continuously.

---

## 5. State Machine Safety

| # | Check | Result | Location |
|---|-------|--------|----------|
| 5a | SW2 emergency stop issues `Wheels_All_Off()`, `DAC_ENB` LOW, IR off, resets all state | **PASS** | interrupt_ports.c switch2_interrupt |
| 5b | Default case in `Run_Project7()` calls `Wheels_All_Off()` | **PASS** | main.c default case |
| 5c | `p7_timer` (unsigned int, max 65535) cannot overflow given maximum 8-tick usage | **PASS** | macros.h EXIT_TURN_TIME=8 is maximum |

---

## 6. Timer / Clock Coupling

| # | Check | Result | Location |
|---|-------|--------|----------|
| 6a | `P7_FOLLOW_LINE` does not mix forward/reverse CCR writes | **PASS** | main.c P7_FOLLOW_LINE |
| 6b | Timer B0 CCR0 math: 8 MHz / 8 / 8 / 25000 = 5 Hz = 200 ms ✓ | **PASS** | timers.c, macros.h |

---

## 7. Other Issues

| # | Check | Result | Location |
|---|-------|--------|----------|
| 7a | No non-volatile globals accessed from ISRs | **PASS** | All ISR files |
| 7b | No missing extern declarations | **PASS** | functions.h, adc.h |
| 7c | No unused globals that could confuse the linker | **PASS** | All source files |

---

## Observation (Low Risk)

**Timer Upper Bound:**
`elapsed_tenths` is `unsigned int` (max 65535). At one increment per 200 ms tick, overflow occurs at 65535 × 0.2 s ≈ 3.6 hours. `TWO_LAP_TIME_TENTHS = 200` (40 seconds maximum lap tracking), so no overflow is possible in normal operation. No action required.

---

## Overall Assessment

**PASS — All critical safety properties satisfied.**

The DAC integration (motor supply via SAC3), H-bridge direction safety, ADC calibration sequencing, interrupt timing, and emergency stop coverage are all correctly implemented for autonomous robot operation.
