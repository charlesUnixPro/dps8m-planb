 //
//  dps8_faults.c
//  dps8
//
//  Created by Harry Reed on 6/11/13.
//  Copyright (c) 2013 Harry Reed. All rights reserved.
//

#include <stdio.h>

#include "dps8.h"
#include "dps8_cpu.h"
#include "dps8_append.h"
#include "dps8_ins.h"
#include "dps8_sys.h"
#include "dps8_utils.h"

#ifndef QUIET_UNUSED
static uint64 FR;
#endif

/*
 FAULT RECOGNITION
 For the discussion following, the term "function" is defined as a major processor functional cycle. Examples are: APPEND CYCLE, CA CYCLE, INSTRUCTION FETCH CYCLE, OPERAND STORE CYCLE, DIVIDE EXECUTION CYCLE. Some of these cycles are discussed in various sections of this manual.
 Faults in groups 1 and 2 cause the processor to abort all functions immediately by entering a FAULT CYCLE.
 Faults in group 3 cause the processor to "close out" current functions without taking any irrevocable action (such as setting PTW.U in an APPEND CYCLE or modifying an indirect word in a CA CYCLE), then to discard any pending functions (such as an APPEND CYCLE needed during a CA CYCLE), and to enter a FAULT CYCLE.
 Faults in group 4 cause the processor to suspend overlapped operation, to complete current and pending functions for the current instruction, and then to enter a FAULT CYCLE.
 Faults in groups 5 or 6 are normally detected during virtual address formation and instruction decode. These faults cause the processor to suspend overlapped operation, to complete the current and pending instructions, and to enter a FAULT CYCLE. If a fault in a higher priority group is generated by the execution of the current or pending instructions, that higher priority fault will take precedence and the group 5 or 6 fault will be lost. If a group 5 or 6 fault is detected during execution of the current instruction (e.g., an access violation, out of segment bounds, fault
 ￼￼
 during certain interruptible EIS instructions), the instruction is considered "complete" upon detection of the fault.
 Faults in group 7 are held and processed (with interrupts) at the completion of the current instruction pair. Group 7 faults are inhibitable by setting bit 28 of the instruction word.
 Faults in groups 3 through 6 must wait for the system controller to acknowledge the last access request before entering the FAULT CYCLE.
 */

/*
 
                                Table 7-1. List of Faults
 
 Decimal fault     Octal (1)      Fault   Fault name            Priority    Group
     number      fault address   mnemonic
        0      ;         0     ;      sdf  ;   Shutdown             ;   27     ;     7
        1      ;         2     ;      str  ;   Store                ;   10     ;     4
        2      ;         4     ;      mme  ;   Master mode entry 1  ;   11     ;     5
        3      ;         6     ;      f1   ;   Fault tag 1          ;   17     ;     5
        4      ;        10     ;      tro  ;   Timer runout         ;   26     ;     7
        5      ;        12     ;      cmd  ;   Command              ;   9      ;     4
        6      ;        14     ;      drl  ;   Derail               ;   15     ;     5
        7      ;        16     ;      luf  ;   Lockup               ;   5      ;     4
        8      ;        20     ;      con  ;   Connect              ;   25     ;     7
        9      ;        22     ;      par  ;   Parity               ;   8      ;     4
        10     ;        24     ;      ipr  ;   Illegal procedure    ;   16     ;     5
        11     ;        26     ;      onc  ;   Operation not complete ; 4      ;     2
        12     ;        30     ;      suf  ;   Startup              ;   1      ;     1
        13     ;        32     ;      ofl  ;   Overflow             ;   7      ;     3
        14     ;        34     ;      div  ;   Divide check         ;   6      ;     3
        15     ;        36     ;      exf  ;   Execute              ;   2      ;     1
        16     ;        40     ;      df0  ;   Directed fault 0     ;   20     ;     6
        17     ;        42     ;      df1  ;   Directed fault 1     ;   21     ;     6
        18     ;        44     ;      df2  ;   Directed fault 2     ;   22     ;     6
        19     ;        46     ;      df3  ;   Directed fault 3     ;   23     ;     6
        20     ;        50     ;      acv  ;   Access violation     ;   24     ;     6
        21     ;        52     ;      mme2 ;   Master mode entry 2  ;   12     ;     5
        22     ;        54     ;      mme3 ;   Master mode entry 3  ;   13     ;     5
        23     ;        56     ;      mme4 ;   Master mode entry 4  ;   14     ;     5
        24     ;        60     ;      f2   ;   Fault tag 2          ;   18     ;     5
        25     ;        62     ;      f3   ;   Fault tag 3          ;   19     ;     5
        26     ;        64     ;           ;   Unassigned           ;          ;
        27     ;        66     ;           ;   Unassigned           ;          ;
 
*/

#ifndef QUIET_UNUSED
static dps8faults _faultsP[] = { // sorted by priority
//  number  address  mnemonic   name                 Priority    Group
    {   12,     030,    "suf",  "Startup",                  1,       1,     false },
    {   15,     036,    "exf",  "Execute",                  2,       1,     false },
    {   31,     076,    "trb",  "Trouble",                  3,       2,     false },
    {   11,     026,    "onc",  "Operation not complete",       4,           2,     false },
    {   7,      016,    "luf",  "Lockup",                       5,           4,     false },
    {   14,     034,    "div",  "Divide check",                 6,           3,     false },
    {   13,     032,    "ofl",  "Overflow",                     7,           3,     false },
    {   9,      022,    "par",  "Parity",                       8,           4,     false },
    {   5,      012,    "cmd",  "Command",                      9,           4,     false },
    {   1,       2 ,    "str",  "Store",                        10,          4,     false },
    {   2,       4 ,    "mme",  "Master mode entry 1",          11,          5,     false },
    {   21,     052,    "mme2", "Master mode entry 2",          12,          5,     false },
    {   22,     054,    "mme3", "Master mode entry 3",          13,          5,     false },
    {   23,     056,    "mme4", "Master mode entry 4",          14,          5,     false },
    {   6,      014,    "drl",  "Derail",                       15,          5,     false },
    {   10,     024,    "ipr",  "Illegal procedure",            16,          5,     false },
    {   3,       06,    "f1",   "Fault tag 1",                  17,          5,     false },
    {   24,     060,    "f2",   "Fault tag 2",                  18,          5,     false },
    {   25,     062,    "f3",   "Fault tag 3",                  19,          5,     false },
    {   16,     040,    "df0",  "Directed fault 0",             20,          6,     false },
    {   17,     042,    "df1",  "Directed fault 1",             21,          6,     false },
    {   18,     044,    "df2",  "Directed fault 2",             22,          6,     false },
    {   19,     046,    "df3",  "Directed fault 3",             23,          6,     false },
    {   20,     050,    "acv",  "Access violation",             24,          6,     false },
    {   8,      020,    "con",  "Connect",                      25,          7,     false },
    {   4,      010,    "tro",  "Timer runout",                 26,          7,     false },
    {   0,       0 ,    "sdf",  "Shutdown",                     27,          7,     false },
    {   26,     064,    "???",  "Unassigned",               -1,     -1,     false },
    {   27,     066,    "???",  "Unassigned",               -1,     -1,     false },
    {   -1,     -1,     NULL,   NULL,                       -1,     -1,     false }
};
#endif
#ifndef QUIET_UNUSED
static dps8faults _faults[] = {    // sorted by number
    //  number  address  mnemonic   name                 Priority    Group
    {   0,       0 ,    "sdf",  "Shutdown",                     27,          7,     false },
    {   1,       2 ,    "str",  "Store",                        10,          4,     false },
    {   2,       4 ,    "mme",  "Master mode entry 1",          11,          5,     false },
    {   3,       06,    "f1",   "Fault tag 1",                  17,          5,     false },
    {   4,      010,    "tro",  "Timer runout",                 26,          7,     false },
    {   5,      012,    "cmd",  "Command",                      9,           4,     false },
    {   6,      014,    "drl",  "Derail",                       15,          5,     false },
    {   7,      016,    "luf",  "Lockup",                       5,           4,     false },
    {   8,      020,    "con",  "Connect",                      25,          7,     false },
    {   9,      022,    "par",  "Parity",                       8,           4,     false },
    {   10,     024,    "ipr",  "Illegal procedure",            16,          5,     false },
    {   11,     026,    "onc",  "Operation not complete",       4,           2,     false },
    {   12,     030,    "suf",  "Startup",                  1,       1,     false },
    {   13,     032,    "ofl",  "Overflow",                     7,           3,     false },
    {   14,     034,    "div",  "Divide check",                 6,           3,     false },
    {   15,     036,    "exf",  "Execute",                  2,       1,     false },
    {   16,     040,    "df0",  "Directed fault 0",             20,          6,     false },
    {   17,     042,    "df1",  "Directed fault 1",             21,          6,     false },
    {   18,     044,    "df2",  "Directed fault 2",             22,          6,     false },
    {   19,     046,    "df3",  "Directed fault 3",             23,          6,     false },
    {   20,     050,    "acv",  "Access violation",             24,          6,     false },
    {   21,     052,    "mme2", "Master mode entry 2",          12,          5,     false },
    {   22,     054,    "mme3", "Master mode entry 3",          13,          5,     false },
    {   23,     056,    "mme4", "Master mode entry 4",          14,          5,     false },
    {   24,     060,    "f2",   "Fault tag 2",                  18,          5,     false },
    {   25,     062,    "f3",   "Fault tag 3",                  19,          5,     false },
    {   26,     064,    "???",  "Unassigned",               -1,     -1,     false },
    {   27,     066,    "???",  "Unassigned",               -1,     -1,     false },
    {   28,     070,    "???",  "Unassigned",               -1,     -1,     false },
    {   29,     072,    "???",  "Unassigned",               -1,     -1,     false },
    {   30,     074,    "???",  "Unassigned",               -1,     -1,     false },
    {   31,     076,    "trb",  "Trouble",                  3,       2,     false },

    {   -1,     -1,     NULL,   NULL,                       -1,     -1,     false }
};
#endif

char * faultNames [N_FAULTS] =
  {
    "Shutdown",
    "Store",
    "Master mode entry 1",
    "Fault tag 1",
    "Timer runout",
    "Command",
    "Derail",
    "Lockup",
    "Connect",
    "Parity",
    "Illegal procedure",
    "Operation not complete",
    "Startup",
    "Overflow",
    "Divide check",
    "Execute",
    "Directed fault 0",
    "Directed fault 1",
    "Directed fault 2",
    "Directed fault 3",
    "Access violation",
    "Master mode entry 2",
    "Master mode entry 3",
    "Master mode entry 4",
    "Fault tag 2",
    "Fault tag 3",
    "Unassigned 26",
    "Unassigned 27",
    "Unassigned 28",
    "Unassigned 29",
    "Unassigned 30",
    "Trouble"
  };
//bool pending_fault = false;     // true when a fault has been signalled, but not processed


#ifndef QUIET_UNUSED
static bool port_interrupts[8] = {false, false, false, false, false, false, false, false };
#endif

//-----------------------------------------------------------------------------
// ***  Constants, unchanging lookup tables, etc

#ifndef QUIET_UNUSED
static int fault2group[32] = {
    // from AL39, page 7-3
    7, 4, 5, 5, 7, 4, 5, 4,
    7, 4, 5, 2, 1, 3, 3, 1,
    6, 6, 6, 6, 6, 5, 5, 5,
    5, 5, 0, 0, 0, 0, 0, 2
};

static int fault2prio[32] = {
    // from AL39, page 7-3
    27, 10, 11, 17, 26,  9, 15,  5,
    25,  8, 16,  4,  1,  7,  6,  2,
    20, 21, 22, 23, 24, 12, 13, 14,
    18, 19,  0,  0,  0,  0,  0,  3
};
#endif
#ifndef QUIET_UNUSED
// Fault conditions as stored in the "FR" Fault Register
// C99 and C++ would allow 64bit enums, but bits past 32 are related to (unimplemented) parity faults.
typedef enum {
    // Values are bit masks
    fr_ill_op = 1, // illegal opcode
    fr_ill_mod = 1 << 1, // illegal address modifier
    // fr_ill_slv = 1 << 2, // illegal BAR mode procedure
    fr_ill_proc = 1 << 3 // illegal procedure other than the above three
    // fr_ill_dig = 1 << 6 // illegal decimal digit
} fault_cond_t;
#endif

#if 0 // DPS8M
// "MR" Mode Register, L68
typedef struct {
    // See member "word" for the raw bits, other member values are derivations
    bool mr_enable; // bit 35 "n"
    bool strobe; // bit 30 "l"
    bool fault_reset; // bit 31 "m"
    uint64 word;
} mode_reg_t;
static mode_reg_t MR;
#endif 

#ifdef NOT_USED
/*
 *  check_events()
 *
 *  Called after executing an instruction pair for xed.   The instruction pair
 *  may have included a rpt, rpd, or transfer.   The instruction pair may even
 *  have faulted, but if so, it was saved and restarted.
 */

void check_events (void)
{
    events.any = events.int_pending || events.low_group || events.group7;
    if (events.any)
      {
        sim_debug(DBG_NOTIFY, & cpu_dev, "CU: check_events: event(s) found (%d,%d,%d).\n", events.int_pending, events.low_group, events.group7);
      }
    return;
}

#if 0
/*
 *  fault_gen()
 *
 *  Called by instructions or the addressing code to record the
 *  existance of a fault condition.
 */

void fault_gen(int f)
{
    int group;
    
#if 0
    if (f == oob_fault) {
        sim_debug(DBG_ERR, & cpu_dev, "CU fault: Faulting for internal bug\n");
        f = FAULT_TRB;
        (void) cancel_run(STOP_BUG);
    }
#endif
    
    if (f < 1 || f > 32) {
        //sim_debug(DBG_ERR, & cpu_dev, "CU fault: Bad fault # %d\n", f);
        cancel_run(STOP_BUG);
        return;
    }
    group = fault2group[f];
    // Note 1-base origin
    if (group < 1 || group > N_FAULT_GROUPS) {
        //sim_debug(DBG_ERR, & cpu_dev, "CU fault: Internal error.\n");
        cancel_run(STOP_BUG);
        return;
    }
    
    if (fault_gen_no_fault) {
        //sim_debug(DBG_DEBUG, & cpu_dev, "CU fault: Ignoring fault # %d in group %d\n", f, group);
        return;
    }
    
    if (f == FAULT_IPR)
        FR |= fr_ill_proc;
    
    events.any = 1;
    //sim_debug(DBG_DEBUG, & cpu_dev, "CU fault: Recording fault # %d in group %d\n", f, group);
    
#if 0 // This is DPS8, not DPS8M
    // Note that we never simulate a (hardware) FAULT_ONC
    if (MR.mr_enable && (f == FAULT_ONC || MR.fault_reset)) {
        if (MR.strobe) {
            sim_debug(DBG_INFO, & cpu_dev, "CU fault: Clearing MR.strobe.\n");
            MR.strobe = 0;
        } else
            sim_debug(DBG_INFO, & cpu_dev, "CU fault: MR.strobe was already unset.\n");
    }
#endif
    
    if (group == 7) {
        // Recognition of group 7 faults is delayed and we can have
        // multiple group 7 faults pending.
        events.group7 |= (1 << f);
    } else {
        // Groups 1-6 are handled more immediately and there can only be
        // one fault pending within each group
        //if (cpu.cycle == FAULT_cycle)
        if (cpu.cycle == FAULT_cycle || cpu.cycle == FAULT_EXEC_cycle) {
            // FIXME: || events.xed AND/OR || cpu.cycle == FAULT_EXEC_cycle
            f = FAULT_TRB;
            group = fault2group[f];
            sim_debug(DBG_WARN, & cpu_dev, "CU fault: Double fault:  Recording current fault as a trouble fault (fault # %d in group %d).\n", f, group);
            cpu.cycle = FAULT_cycle;
            //cancel_run(STOP_DIS); // BUG: not really
        } else {
// XXX error? if fault[] 0-origin or 1-origin?
// XXX ticket #8
            if (events.fault[group]) {
                // todo: error, unhandled fault
                sim_debug(DBG_WARN, & cpu_dev, "CU fault: Found unhandled prior fault #%d in group %d.\n", events.fault[group], group);
            }
            if (cpu.cycle == EXEC_cycle) {
                // don't execute any pending odd half of an instruction pair
                cpu.cycle = FAULT_cycle;
            }
        }
        events.fault[group] = f;
    }
    if (events.low_group == 0 || group < events.low_group)
        events.low_group = group;   // new highest priority fault group
}
#endif

/*
 * fault_check_group
 *
 * Returns true if faults exist for the specifed group or for a higher
 * priority group.
 *
 */

#ifndef QUIET_UNUSED
static int fault_check_group(int group)
{
    // Note 1-origin
    if (group < 1 || group > N_FAULT_GROUPS) {
        sim_debug(DBG_ERR, & cpu_dev, "CU fault-check-group: Bad group # %d\n", group);
        sim_err("CU fault-check-group: Bad group # %d\n", group); // Doesn't return
    }
    
    if (! events.any)
        return 0;
    return events.low_group <= group;
}
#endif

#endif  // NOT_USED

/*
 * fault handler(s).
 */

static bool bTroubleFaultCycle = false;       // when true then in TROUBLE FAULT CYCLE
#ifndef QUIET_UNUSED
static int nFaultNumber = -1;
static int nFaultGroup = -1;
static int nFaultPriority = -1;
#endif
static uint g7Faults = 0;
static _fault_subtype  g7SubFaults [N_FAULTS];

// We stash a few things for debugging; they are accessed by emCall.
static word18 fault_ic; 
static word15 fault_psr;
static char fault_msg [1024];


void emCallReportFault (void)
  {
           sim_printf ("fault report:\n");
           sim_printf ("  fault number %d (%o)\n", cpu . faultNumber, cpu . faultNumber);
           sim_printf ("  subfault number %d (%o)\n", cpu . subFault, cpu . subFault);
           sim_printf ("  faulting address %05o:%06o\n", fault_psr, fault_ic);
           sim_printf ("  msg %s\n", fault_msg);
  }

void clearFaultCycle (void)
  {
    bTroubleFaultCycle = false;
  }

/*

 Faults in groups 1 and 2 cause the processor to abort all functions immediately by entering a FAULT CYCLE.
 
 Faults in group 3 cause the processor to "close out" current functions without taking any irrevocable action (such as setting PTW.U in an APPEND CYCLE or modifying an indirect word in a CA CYCLE), then to discard any pending functions (such as an APPEND CYCLE needed during a CA CYCLE), and to enter a FAULT CYCLE.
 
 Faults in group 4 cause the processor to suspend overlapped operation, to complete current and pending functions for the current instruction, and then to enter a FAULT CYCLE.
 
 Faults in groups 5 or 6 are normally detected during virtual address formation and instruction decode. These faults cause the processor to suspend overlapped operation, to complete the current and pending instructions, and to enter a FAULT CYCLE. If a fault in a higher priority group is generated by the execution of the current or pending instructions, that higher priority fault will take precedence and the group 5 or 6 fault will be lost. If a group 5 or 6 fault is detected during execution of the current instruction (e.g., an access violation, out of segment bounds, fault during certain interruptible EIS instructions), the instruction is considered "complete" upon detection of the fault.
 
 Faults in group 7 are held and processed (with interrupts) at the completion of the current instruction pair.
 
 Group 7 faults are inhibitable by setting bit 28 of the instruction word.
 
 Faults in groups 3 through 6 must wait for the system controller to acknowledge the last access request before entering the FAULT CYCLE.
 

After much rumination here are my thoughts for fault processing .....

For now, at least, we must remember a few things:

1) We only have 1 cpu so we have few & limited async faults - shutdown, TRO, etc.
2) We have no overlapping instruction execution
3) Becuase of 2) we have no pending instructions
4) We have no system controller to wait for
 
 Group 1 & 2 faults can be processed immediately and then proceed to next instruction as long as no transfer prevents us from returing from the XED pair.
 
 Group 3 faults will probably also execute immediately since a G3 fault causes "the processor to "close out" current functions without taking any irrevocable action (such as setting PTW.U in an APPEND CYCLE or modifying an indirect word in a CA CYCLE), then to discard any pending functions (such as an APPEND CYCLE needed during a CA CYCLE), and to enter a FAULT CYCLE."
 
 Group 4 faults will probably also execute immediately since a G4 fault causes "the processor to suspend overlapped operation, to complete current and pending functions for the current instruction, and then to enter a FAULT CYCLE."

 Group 5 & 6 faults will probably also execute immediately because "if a group 5 or 6 fault is detected during execution of the current instruction (e.g., an access violation, out of segment bounds, fault during certain interruptible EIS instructions), the instruction is considered "complete" upon detection of the fault." However, remember "If a fault in a higher priority group is generated by the execution of the current or pending instructions, that higher priority fault will take precedence and the group 5 or 6 fault will be lost. If a group 5 or 6 fault is detected during execution of the current instruction (e.g., an access violation, out of segment bounds, fault during certain interruptible EIS instructions), the instruction is considered "complete" upon detection of the fault."
 ￼￼
 during certain interruptible EIS instructions), the instruction is considered "complete" upon detection of the fault.

 For furter justification of immediate execution since "Faults in groups 3 through 6 must wait for the system controller to acknowledge the last access request before entering the FAULT CYCLE."
 
 Group 7 faults will be processed after next even instruction decode instruction decode, but before instruction execution. In this way we can actually use bit-28 tp inhibit interrupts
 
*/

// CANFAULT 
void doFault(_fault faultNumber, _fault_subtype subFault, const char *faultMsg)
{
    sim_debug (DBG_FAULT, & cpu_dev, "Fault %d(0%0o), sub %d(0%o), dfc %c, '%s'\n", faultNumber, faultNumber, subFault, subFault, bTroubleFaultCycle ? 'Y' : 'N', faultMsg);
#if 0
    if (faultNumber == FAULT_ACV && subFault == ACV13)
      {
        if_sim_debug (DBG_CAC, & cpu_dev)
          {
            sim_debug (DBG_CAC, & cpu_dev, "RALR fault\n");
            traceInstruction (DBG_CAC);
          }
      }
#endif
    if_sim_debug (DBG_FAULT, & cpu_dev)
      traceInstruction (DBG_FAULT);

#if 0
if (faultNumber == 10 && sys_stats . total_cycles > 10000)
  {
    stop_reason = STOP_HALT;
    longjmp (jmpMain, JMP_STOP);
  }
#endif

    // some debugging support stuff
    fault_psr = PPR.PSR;
    fault_ic = PPR.IC;
    strcpy (fault_msg, faultMsg);

    //if (faultNumber < 0 || faultNumber > 31)
    if (faultNumber & ~037U)  // quicker?
    {
        sim_printf("fault(out-of-range): %d %d '%s'\n", faultNumber, subFault, faultMsg ? faultMsg : "?");
        sim_err ("fault out-of-range\n");
    }

    cpu . faultNumber = faultNumber;
    cpu . subFault = subFault;
    sys_stats . total_faults [faultNumber] ++;

    // Set fault register bits

    if (faultNumber == FAULT_IPR)
      {
        if (subFault == ill_op)
          faultRegister [0] |= FR_ILL_OP;
        else if (subFault == ill_mod)
          faultRegister [0] |= FR_ILL_MOD;
        else if (subFault == ill_dig)
          faultRegister [0] |= FR_ILL_DIG;
        else /* if (subFault == ill_proc) */ // and all others
          faultRegister [0] |= FR_ILL_PROC;
      }
    else if (faultNumber == FAULT_ONC && subFault == nem)
      {
        faultRegister [0] |= FR_NEM;
      }
    else if (faultNumber == FAULT_STR && subFault == oob)
      {
        faultRegister [0] |= FR_OOB;
      }
    else if (faultNumber == FAULT_CON)
      {
        switch (subFault)
          {
            case 0:
              faultRegister [0] |= FR_CON_A;
              break;
            case 1:
              faultRegister [0] |= FR_CON_B;
              break;
            case 2:
              faultRegister [0] |= FR_CON_C;
              break;
            case 3:
              faultRegister [0] |= FR_CON_D;
              break;
            default:
              break;
          }
      }

    // Set cu word1 fault bits

    cu . IRO_ISN = 0;
    cu . OEB_IOC = 0;
    cu . EOFF_IAIM = 0;
    cu . ORB_ISP = 0;
    cu . ROFF_IPR = 0;
    cu . OWB_NEA = 0;
    cu . WOFF_OOB = 0;
    cu . NO_GA = 0;
    cu . OCB = 0;
    cu . OCALL = 0;
    cu . BOC = 0;
    cu . PTWAM_ER = 0;
    cu . CRT = 0;
    cu . RALR = 0;
    cu . SWWAM_ER = 0;
    cu . OOSB = 0;
    cu . PARU = 0;
    cu . PARL = 0;
    cu . ONC1 = 0;
    cu . ONC2 = 0;
    cu . IA = 0;
    cu . IACHN = 0;
    cu . CNCHN = 0;

    // Set control unit 'fault occured during instruction fetch' flag
    cu . FIF = cpu . cycle == FETCH_cycle ? 1 : 0;
    cu . FI_ADDR = faultNumber;

    if (faultNumber == FAULT_ACV)
      {
        // This is annoyingly inefficent since the subFault value 
        // is bitwise the same as the upper half of CU word1;
        // if the upperhalf were not broken out, then this would be
        // cu . word1_upper_half = subFault.

        if (subFault & ACV0)
          cu . IRO_ISN = 1;
        if (subFault & ACV1)
          cu . OEB_IOC = 1;
        if (subFault & ACV2)
          cu . EOFF_IAIM = 1;
        if (subFault & ACV3)
          cu . ORB_ISP = 1;
        if (subFault & ACV4)
          cu . ROFF_IPR = 1;
        if (subFault & ACV5)
          cu . OWB_NEA = 1;
        if (subFault & ACV6)
          cu . WOFF_OOB = 1;
        if (subFault & ACV7)
          cu . NO_GA = 1;
        if (subFault & ACV8)
          cu . OCB = 1;
        if (subFault & ACV9)
          cu . OCALL = 1;
        if (subFault & ACV10)
          cu . BOC = 1;
        if (subFault & ACV11)
          cu . PTWAM_ER = 1;
        if (subFault & ACV12)
          cu . CRT = 1;
        if (subFault & ACV13)
          cu . RALR = 1;
        if (subFault & ACV14)
          cu . SWWAM_ER = 1;
        if (subFault & ACV15)
          cu . OOSB = 1;
      }
    else if (faultNumber == FAULT_STR)
      {
        if (subFault == oob)
          cu . WOFF_OOB = 1;
        else if (subFault == ill_ptr)
          cu . WOFF_OOB = 1;
        // Not used by SCU 4MW
        // else if (subFault == not_control)
          // cu . WOFF_OOB;
      }
    else if (faultNumber == FAULT_IPR)
      {
        if (subFault == ill_op)
          cu . OEB_IOC = 1;
        else if (subFault == ill_mod)
          cu . EOFF_IAIM = 1;
        else if (subFault == ill_slv)
          cu . ORB_ISP = 1;
        else if (subFault == ill_dig)
          cu . ROFF_IPR = 1;
        // else if (subFault == ill_proc)
          // cu . ? = 1;
      }
    // If already in a FAULT CYCLE then signal trouble fault

    if (cpu . cycle == FAULT_EXEC_cycle ||
        cpu . cycle == FAULT_EXEC2_cycle)
      {
        cpu . faultNumber = FAULT_TRB;
        cu . FI_ADDR = FAULT_TRB;
        cpu . subFault = 0; // XXX ???
        // XXX Does the CU or FR need fixing? ticket #36
        if (bTroubleFaultCycle)
          {
            if ((! sample_interrupts ()) &&
                (sim_qcount () == 0))  // XXX If clk_svc is implemented it will 
                                     // break this logic
              {
                sim_printf ("Fault cascade @0%06o with no interrupts pending and no events in queue\n", PPR.IC);
                sim_printf("\nsimCycles = %lld\n", sim_timell ());
                sim_printf("\ncpuCycles = %lld\n", sys_stats . total_cycles);
                //stop_reason = STOP_FLT_CASCADE;
                longjmp (jmpMain, JMP_STOP);
              }
          }
        else
          {
//--            f = &_faults[FAULT_TRB];
            bTroubleFaultCycle = true;
          }
      }
    else
      {
        bTroubleFaultCycle = false;
      }
    
    // If doInstruction faults, the instruction cycle counter doesn't get 
    // bumped.
    if (cpu . cycle == EXEC_cycle)
      sys_stats . total_cycles += 1; // bump cycle counter

    cpu . cycle = FAULT_cycle;
    sim_debug (DBG_CYCLE, & cpu_dev, "Setting cycle to FAULT_cycle\n");
    longjmp (jmpMain, JMP_REENTRY);
}

//
// return true if group 7 faules are pending ...
//

// Note: The DIS code assumes that the only G7 fault is TRO. Adding any
// other G7 faults will potentailly require changing the DIS code.
 
bool bG7Pending (void)
  {
    return g7Faults != 0;
  }

bool bG7PendingNoTRO (void)
  {
    return (g7Faults & (~ (1u << FAULT_TRO))) != 0;
  }

void setG7fault (_fault faultNo, _fault_subtype subFault)
  {
    // sim_printf ("setG7fault %d %d [%lld]\n", faultNo, subFault, sim_timell ());
    sim_debug (DBG_FAULT, & cpu_dev, "setG7fault %d %d\n", faultNo, subFault);
    g7Faults |= (1u << faultNo);
    g7SubFaults [faultNo] = subFault;
  }

void clearTROFault (void)
  {
    g7Faults &= ~(1u << FAULT_TRO);
  }

void doG7Fault (void)
  {
    // sim_printf ("doG7fault %08o [%lld]\n", g7Faults, sim_timell ());
    // if (g7Faults)
      // {
        // sim_debug (DBG_FAULT, & cpu_dev, "doG7Fault %08o\n", g7Faults);
      // }
     if (g7Faults & (1u << FAULT_TRO))
       {
         g7Faults &= ~(1u << FAULT_TRO);

         doFault (FAULT_TRO, 0, "Timer runout"); 
       }

     if (g7Faults & (1u << FAULT_CON))
       {
         g7Faults &= ~(1u << FAULT_CON);

         cu . CNCHN = g7SubFaults [FAULT_CON] & MASK3;
         doFault (FAULT_CON, g7SubFaults [FAULT_CON], "Connect"); 
       }

     // Strictly speaking EXF isn't a G7 fault, put if we treat is as one,
     // we are allowing the current instruction to complete, simplifying
     // implementation
     if (g7Faults & (1u << FAULT_EXF))
       {
         g7Faults &= ~(1u << FAULT_EXF);

         doFault (FAULT_EXF, 0, "Execute fault");
       }

     doFault (FAULT_TRB, (_fault_subtype) g7Faults, "Dazed and confused in doG7Fault");
  }
