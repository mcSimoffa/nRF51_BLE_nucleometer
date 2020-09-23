/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*            (c) 2014 - 2020 SEGGER Microcontroller GmbH             *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* Redistribution and use in source and binary forms, with or         *
* without modification, are permitted provided that the following    *
* condition is met:                                                  *
*                                                                    *
* - Redistributions of source code must retain the above copyright   *
*   notice, this condition and the following disclaimer.             *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL SEGGER Microcontroller BE LIABLE FOR *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
* DAMAGE.                                                            *
*                                                                    *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------

File      : SEGGER_THUMB_Startup.s
Purpose   : Generic runtime init startup code for ARM CPUs running 
            in THUMB mode.
            Designed to work with the SEGGER linker to produce 
            smallest possible executables.
            
            This file does not normally require any customization.

Additional information:
  Preprocessor Definitions
    FULL_LIBRARY
      If defined then 
        - argc, argv are setup by the debug_getargs.
        - the exit symbol is defined and executes on return from main.
        - the exit symbol calls destructors, atexit functions and then debug_exit.
    
      If not defined then
        - argc and argv are not valid (main is assumed to not take parameters)
        - the exit symbol is defined, executes on return from main and loops
*/

        .syntax unified  

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#ifndef   APP_ENTRY_POINT
  #define APP_ENTRY_POINT main
#endif

#ifndef   ARGSSPACE
  #define ARGSSPACE 128
#endif

/*********************************************************************
*
*       Externals
*
**********************************************************************
*/
        .extern APP_ENTRY_POINT     // typically main

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/
/*********************************************************************
*
*       _start
*
*  Function description
*    Entry point for the startup code. 
*    Usually called by the reset handler.
*    Performs all initialisation, based on the entries in the 
*    linker-generated init table, then calls main().
*    It is device independent, so there should not be any need for an 
*    end-user to modify it.
*
*  Additional information
*    At this point, the stack pointer should already have been 
*    initialized:
*      - For typical programs executiong from flash, the stack pointer 
*        has already been loaded from vector table at offset 0.
*      - In other cases, such as RAM Code, this is done by the debugger 
*        or the device-specific startup code / reset handler.
*/
        .section .init, "ax"
        .balign 2
        .global _start
        .thumb_func
        .code 16
_start:
        //
        // Call linker init functions which in turn performs the following:
        // * Perform segment init
        // * Perform heap init (if used)
        // * Call constructors of global Objects (if any exist)
        //
        ldr     R4, =__SEGGER_init_table__      // Set table pointer to start of initialization table
__SEGGER_init_run_loop: 
        ldr     R0, [R4]                        // Get next initialization function from table
        adds    R4, R4, #4                      // Increment table pointer to point to function arguments
        blx     R0                              // Call initialization function
        b.n     __SEGGER_init_run_loop
        //
       .thumb_func
       .global __SEGGER_init_done
__SEGGER_init_done:
        //
        // Time to call main(), the application entry point.
        //
#ifndef FULL_LIBRARY
        //
        // In a real embedded application ("Free-standing environment"), 
        // main() does not get any arguments,
        // which means it is not necessary to init R0 and R1.
        //
        bl      APP_ENTRY_POINT                 // Call to application entry point (usually main())
        //
        // Fall-through to exit if main ever returns.
        // 
        .global exit
        .thumb_func
exit:
        //
        // In a free-standing environment, if returned from application:
        // Loop forever.
        //
        b       .
#else
        //
        // In a hosted environment, 
        // we need to load R0 and R1 with argc and argv, in order to handle 
        // the command line arguments.
        // This is required for some programs running under control of a 
        // debugger, such as automated tests.
        //
        movs    R0, #ARGSSPACE
        ldr     R1, =args
        bl      SEGGER_SEMIHOST_GetArgs
        ldr     R1, =args
        bl      APP_ENTRY_POINT                 // Call to application entry point (usually main())
        //
        // Fall-through to exit if main ever returns.
        // 
        .global exit
        .thumb_func
exit:
        //
        // In a hosted environment exit gracefully, by
        // saving the return value,
        // calling destructurs of global objects, 
        // calling registered atexit functions, 
        // and notifying the host/debugger.
        //
        mov     R5, R0                  // Save the exit parameter/return result
        //
        // Call destructors
        //
        ldr     R0, =__dtors_start__    // Pointer to destructor list
        ldr     R1, =__dtors_end__
dtorLoop:
        cmp     R0, R1
        beq     dtorEnd                 // Reached end of destructor list? => Done
        ldr     R2, [R0]                // Load current destructor address into R2
        add     R0, #4                  // Increment pointer
        push    {R0-R1}                 // Save R0 and R1
        blx     R2                      // Call destructor
        pop     {R0-R1}                 // Restore R0 and R1
        b       dtorLoop
dtorEnd:
        //
        // Call atexit functions
        //
        bl      __SEGGER_RTL_execute_at_exit_fns
        //
        // Call debug_exit with return result/exit parameter
        //
        mov     R0, R5
        bl      SEGGER_SEMIHOST_Exit
        //
        // If execution is not terminated, loop forever
        //
exit_loop:
        b       exit_loop // Loop forever.
#endif

#ifdef FULL_LIBRARY
        .bss
args:
        .space ARGSSPACE
#endif

/*************************** End of file ****************************/
