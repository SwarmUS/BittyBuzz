#ifndef BBZVM_H
#define BBZVM_H

#include <inttypes.h>

#include "bbzheap.h"
#include "bbzdarray.h"
#include "bbztable.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus



    // ======================================
    // =       DATA TYPE DEFNINITIONS       =
    // ======================================



    /**
     * @brief VM states
     */
    typedef enum {
        BBZVM_STATE_NOCODE = 0, /**< @brief No code loaded */
        BBZVM_STATE_READY,      /**< @brief Ready to execute next instruction */
        BBZVM_STATE_DONE,       /**< @brief Program finished */
        BBZVM_STATE_ERROR,      /**< @brief Error occurred */
        BBZVM_STATE_STOPPED     /**< @brief Stopped due to a breakpoint */
    } bbzvm_state;

    /**
     * @brief VM error codes
     */
    typedef enum {
        BBZVM_ERROR_NONE = 0, /**< @brief No error */
        BBZVM_ERROR_INSTR,    /**< @brief Unknown instruction  */
        BBZVM_ERROR_STACK,    /**< @brief Stack error */
        BBZVM_ERROR_LNUM,     /**< @brief Wrong number of local variables */
        BBZVM_ERROR_PC,       /**< @brief Program counter out of range */
        BBZVM_ERROR_FLIST,    /**< @brief Function call id out of range */
        BBZVM_ERROR_TYPE,     /**< @brief Type mismatch */
        BBZVM_ERROR_STRING,   /**< @brief Unknown string id */
        BBZVM_ERROR_SWARM     /**< @brief Unknown swarm id */
    } bbzvm_error;

    /**
     * @brief VM instructions
     */
    typedef enum {
        /**
         * Opcodes without argument
         */
        BBZVM_INSTR_NOP = 0, /**< @brief No operation */
        BBZVM_INSTR_DONE,    /**< @brief End of the program */
        BBZVM_INSTR_PUSHNIL, /**< @brief Push nil onto stack */
        BBZVM_INSTR_DUP,     /**< @brief Duplicate stack top */
        BBZVM_INSTR_POP,     /**< @brief Pop value from stack */
        BBZVM_INSTR_RET0,    /**< @brief Returns from closure call, see bbzvm_ret0() */
        BBZVM_INSTR_RET1,    /**< @brief Returns from closure call, see bbzvm_ret1() */
        BBZVM_INSTR_ADD,     /**< @brief Push stack(#0) + stack(#1), pop operands */
        BBZVM_INSTR_SUB,     /**< @brief Push stack(#0) - stack(#1), pop operands */
        BBZVM_INSTR_MUL,     /**< @brief Push stack(#0) * stack(#1), pop operands */
        BBZVM_INSTR_DIV,     /**< @brief Push stack(#0) / stack(#1), pop operands */
        BBZVM_INSTR_MOD,     /**< @brief Push stack(#0) % stack(#1), pop operands */
        BBZVM_INSTR_POW,     /**< @brief Push stack(#0) ^ stack(#1), pop operands */
        BBZVM_INSTR_UNM,     /**< @brief Push -stack(#0), pop operand */
        BBZVM_INSTR_AND,     /**< @brief Push stack(#0) & stack(#1), pop operands */
        BBZVM_INSTR_OR,      /**< @brief Push stack(#0) | stack(#1), pop operands */
        BBZVM_INSTR_NOT,     /**< @brief Push !stack(#0), pop operand */
        BBZVM_INSTR_EQ,      /**< @brief Push stack(#0) == stack(#1), pop operands */
        BBZVM_INSTR_NEQ,     /**< @brief Push stack(#0) != stack(#1), pop operands */
        BBZVM_INSTR_GT,      /**< @brief Push stack(#0) > stack(#1), pop operands */
        BBZVM_INSTR_GTE,     /**< @brief Push stack(#0) >= stack(#1), pop operands */
        BBZVM_INSTR_LT,      /**< @brief Push stack(#0) < stack(#1), pop operands */
        BBZVM_INSTR_LTE,     /**< @brief @brief/ Push stack(#0) <= stack(#1), pop operands */
        BBZVM_INSTR_GLOAD,   /**< @brief Push global variable corresponding to string at stack #0, pop operand */
        BBZVM_INSTR_GSTORE,  /**< @brief Store stack-top value into global variable at stack #1, pop operands */
        BBZVM_INSTR_PUSHT,   /**< @brief Push empty table */
        BBZVM_INSTR_TPUT,    /**< @brief Put key (stack(#1)), value (stack #0) in table (stack #2), pop key and value */
        BBZVM_INSTR_TGET,    /**< @brief Push value for key (stack(#0)) in table (stack #1), pop key */
        BBZVM_INSTR_CALLC,   /**< @brief Calls the closure on top of the stack as a normal closure */
        BBZVM_INSTR_CALLS,   /**< @brief Calls the closure on top of the stack as a swarm closure */
        /**
         * Opcodes with argument
         */
        /* Float argument */
        BBZVM_INSTR_PUSHF,   /**< @brief Push float constant onto stack */
        /* Integer argument */
        BBZVM_INSTR_PUSHI,   /**< @brief Push integer constant onto stack */
        BBZVM_INSTR_PUSHS,   /**< @brief Push string constant onto stack */
        BBZVM_INSTR_PUSHCN,  /**< @brief Push native closure onto stack */
        BBZVM_INSTR_PUSHCC,  /**< @brief Push c-function closure onto stack */
        BBZVM_INSTR_PUSHL,   /**< @brief Push native closure lambda onto stack */
        BBZVM_INSTR_LLOAD,   /**< @brief Push local variable at given position */
        BBZVM_INSTR_LSTORE,  /**< @brief Store stack-top value into local variable at given position, pop operand */
        BBZVM_INSTR_JUMP,    /**< @brief Set PC to argument */
        BBZVM_INSTR_JUMPZ,   /**< @brief Set PC to argument if stack top is zero, pop operand */
        BBZVM_INSTR_JUMPNZ,  /**< @brief Set PC to argument if stack top is not zero, pop operand */
        BBZVM_INSTR_COUNT    /**< @brief Used to count how many instructions have been defined */
    } bbzvm_instr;

    /**
     * @brief Type for the pointer to a function which fetches bytecode data.
     * @warning The function provider should take endianness
     * into account if copying byte-by-byte.
     * @param[in] offset Bytecode offset for the data to fetch.
     * @param[in] size Size of the data to set. The VM will at most
     * try to read 4 bytes (uint32_t). FIXME No, only reads 1 byte at a time.
     * @return A pointer to the data to the data.
     */
    typedef const uint8_t* (*bbzvm_bcode_fetch_fun)(uint16_t offset, uint8_t size);

    /**
     * @brief Type for the pointer to a function that is called whenever the
     * VM meets an error.
     * @details This function may perform operations based on the type of the
     * errors that occur. Typically, this involves notifying the outside world
     * that an error has occurred (such as stopping the actuators and sending a
     * signal), or handling the error manually.
     * @param[in] errcode The type of the error that occurred.
     */
    typedef void (*bbzvm_error_notifier_fun)(bbzvm_error errcode);

    /**
     * @brief The BittyBuzz Virtual Machine.
     * 
     * @details Responsibilities:
     *      1) Load and run bytecode.
     */
    typedef struct __attribute__((packed)) {
        /** @brief Bytecode fetcher function */
        bbzvm_bcode_fetch_fun bcode_fetch_fun;
        /** @brief Size of the loaded bytecode */
        uint16_t bcode_size;
        /** @brief Program counter */
        int16_t pc;
        /** @brief Current stack content */
        bbzheap_idx_t stack[BBZSTACK_SIZE];
        /** @brief Stack pointer (Index of the last valid element of the stack) */
        int16_t stackptr;
        /** @brief Block pointer (... ?) */
        int16_t blockptr;
        /** @brief Current local variable table */
        bbzheap_idx_t lsyms;
        /** @brief Local variable table list */
        bbzheap_idx_t lsymts;
        /** @brief Global symbols */
        bbzheap_idx_t gsyms;
        /* Strings */
        // TODO
        /** @brief Heap content */
        bbzheap_t heap;
        /** @brief Singleton bbznil_t */
        bbzheap_idx_t nil;
        /** @brief Singleton bbzdarray_t for the default activations record */
        bbzheap_idx_t dflt_actrec;
        /** @brief Registered functions */
        bbzheap_idx_t flist;
        /* List of known swarms */
        // TODO
        /* List of known swarms */
        // TODO
        /* Swarm members */
        // TODO
        /* Counter for swarm membership broadcasting */
        // TODO
        /* Input message FIFO */
        // TODO
        /* Output message FIFO */
        // TODO
        /* Virtual stigmergy maps */
        // TODO
        /* Neighbor value listeners */
        // TODO
        /** @brief Current VM state */
        bbzvm_state state;
        /** @brief Current VM error */
        bbzvm_error error;
        /** @brief Error notifier. */
        bbzvm_error_notifier_fun error_notifier_fun;
        /* Current VM error message */
        // TODO
        /** @brief Robot id */
        uint16_t robot;
        /* Random number generator state */
        // TODO
        /* Random number generator index */
        // TODO
    } bbzvm_t;

    typedef int (*bbzvm_funp)(bbzvm_t* vm);



    // ======================================
    // =  GENERAL VM FUNCTION DEFNINITIONS  =
    // ======================================



    /**
     * @brief Sets up a VM.
     * @param robot The robot id.
     * @return The VM.
     */
    void bbzvm_construct(bbzvm_t* vm, uint16_t robot);

    /**
     * @brief Destroys the VM.
     * @param vm The VM.
     */
    void bbzvm_destruct(bbzvm_t* vm);

    /**
     * @brief Sets the error state of the VM.
     * @param vm The VM.
     * @param errcode The code of the error.
     * @see bbzvm_error
     */
    void bbzvm_seterror(bbzvm_t* vm, bbzvm_error errcode);

    /**
     * @brief Sets the bytecode function in the VM.
     * @warning The passed buffer should not be deleted until the VM is done with it.
     * @warning The function provider should take endianness
     * into account if copying byte-by-byte.
     * @param[in,out] vm The VM.
     * @param[in] bcode_fetch_fun The function to call to read bytecode data.
     * @param[in] bcode_size The size (in bytes) of the bytecode.
     * @return 0 if everything OK, a non-zero value in case of error
     */
    int bbzvm_set_bcode(bbzvm_t* vm, bbzvm_bcode_fetch_fun bcode_fetch_fun, uint32_t bcode_size);

    /**
     * @brief Sets the error notifier.
     * @see bbzvm_error_notifier_fun
     * @param[in,out] vm The VM.
     * @param[in]
     */
     __attribute__((always_inline)) inline
    void bbzvm_set_error_notifier(bbzvm_t* vm, bbzvm_error_notifier_fun error_notifier_fun) {
        vm->error_notifier_fun = error_notifier_fun;
    }

    /**
     * @brief Processes the input message queue.
     * @param vm The VM.
     */
    void bbzvm_process_inmsgs(bbzvm_t* vm);

    /**
     * @brief Processes the output message queue.
     * @param vm The VM.
     */
    void bbzvm_process_outmsgs(bbzvm_t* vm);

    /**
     * @brief Executes the next step in the bytecode, if possible.
     * @details Should there be an error during stepping, the VM's
     * program counter will point to the instruction that caused the error.
     * @param vm The VM.
     * @return The updated VM state.
     */
    bbzvm_state bbzvm_step(bbzvm_t* vm);

    /**
     * @brief Executes the script up to completion.
     * @param vm The VM.
     * @return The updated VM state.
     */
    bbzvm_state bbzvm_execute_script(bbzvm_t* vm);
    

    // ======================================
    // =         BYTECODE FUNCTIONS         =
    // ======================================

    /**
     * @brief Terminates the current Buzz script.
     * @details This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @see BBZVM_INSTR_DONE
     * @param[in,out] vm The VM data.
     * @return The updated state of the VM.
     */
     __attribute((always_inline)) inline
    bbzvm_state bbzvm_done(bbzvm_t* vm) {
        (vm)->state = BBZVM_STATE_DONE;
        return BBZVM_STATE_DONE;
    }

    /**
     * @brief Pushes nil on the stack.
     * @see BBZVM_INSTR_PUSHNIL
     * @param vm The VM.
     * @return The VM state.
     */
    bbzvm_state bbzvm_pushnil(bbzvm_t* vm);

    /**
     * @brief Duplicates the current stack top.
     * @see BBZVM_INSTR_DUP
     * @param vm The VM.
     * @return The VM state.
     */
    bbzvm_state bbzvm_dup(bbzvm_t* vm);

    /**
     * @brief Pops the stack.
     * @details Internally checks whether the operation is valid.
     * @see BBZVM_INSTR_POP
     * @param vm The VM.
     * @return The VM state.
     */
    bbzvm_state bbzvm_pop(bbzvm_t* vm);

    /**
     * @brief Returns from a closure without setting a return value.
     * @details Internally checks whether the operation is valid.
     * 
     * This function expects at least two stacks to be present. The
     * first stack is popped. The stack beneath, now the top stack, is
     * expected to have at least one element: the return address at
     * #1. The return address is popped and used to update the program
     * counter.
     * @see BBZVM_INSTR_RET0
     * @param vm The VM.
     * @return The VM state.
     */
    bbzvm_state bbzvm_ret0(bbzvm_t* vm);

    /**
     * @brief Returns from a closure setting a return value.
     * @details Internally checks whether the operation is valid.
     * 
     * This function expects at least two stacks to be present. The
     * first stack must have at least one element, which is saved as
     * the return value of the call. The stack is then popped. The
     * stack beneath, now the top stack, is expected to have at least
     * one element: the return address at #1. The return address is
     * popped and used to update the program counter. Then, the saved
     * return value is pushed on the stack.
     * @see BBZVM_INSTR_RET1
     * @param vm The VM.
     * @return The VM state.
     */
    bbzvm_state bbzvm_ret1(bbzvm_t* vm);

    /**
     * @brief Performs an addition.
     * @details This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @see BBZVM_INSTR_ADD
     * @param[in,out] vm The VM.
     * @return The updated state of the VM.
     */
    bbzvm_state bbzvm_add(bbzvm_t* vm);

    /**
     * @brief Performs an subtraction.
     * @details This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @see BBZVM_INSTR_SUB
     * @param[in,out] vm The VM.
     * @return The updated state of the VM.
     */
    bbzvm_state bbzvm_sub(bbzvm_t* vm);

    /**
     * @brief Performs an multiplication.
     * @details This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @see BBZVM_INSTR_MUL
     * @param[in,out] vm The VM.
     * @return The updated state of the VM.
     */
    bbzvm_state bbzvm_mul(bbzvm_t* vm);

    /**
     * @brief Performs an division.
     * @details This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @see BBZVM_INSTR_DIV
     * @param[in,out] vm The VM.
     * @return The updated state of the VM.
     */
    bbzvm_state bbzvm_div(bbzvm_t* vm);

    /**
     * @brief Performs an subtraction.
     * @details This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @see BBZVM_INSTR_MOD
     * @param[in,out] vm The VM.
     * @return The updated state of the VM.
     */
    bbzvm_state bbzvm_mod(bbzvm_t* vm);

    /**
     * @brief Performs an subtraction.
     * @details This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @see BBZVM_INSTR_POW
     * @param[in,out] vm The VM.
     * @return The updated state of the VM.
     */
    bbzvm_state bbzvm_pow(bbzvm_t* vm);

    /**
     * @brief Performs an subtraction.
     * @details This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @see BBZVM_INSTR_UNM
     * @param[in,out] vm The VM.
     * @return The updated state of the VM.
     */
    bbzvm_state bbzvm_unm(bbzvm_t* vm);

    /**
     * @brief Performs an subtraction.
     * @details This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @see BBZVM_INSTR_AND
     * @param[in,out] vm The VM.
     * @return The updated state of the VM.
     */
    bbzvm_state bbzvm_and(bbzvm_t* vm);

    /**
     * @brief Performs an subtraction.
     * @details This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @see BBZVM_INSTR_OR
     * @param[in,out] vm The VM.
     * @return The updated state of the VM.
     */
    bbzvm_state bbzvm_or(bbzvm_t* vm);

    /**
     * @brief Performs an subtraction.
     * @details This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @see BBZVM_INSTR_NOT
     * @param[in,out] vm The VM.
     * @return The updated state of the VM.
     */
    bbzvm_state bbzvm_not(bbzvm_t* vm);

    /**
     * @brief Performs an subtraction.
     * @details This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @see BBZVM_INSTR_EQ
     * @param[in,out] vm The VM.
     * @return The updated state of the VM.
     */
    bbzvm_state bbzvm_eq(bbzvm_t* vm);

    /**
     * @brief Performs an subtraction.
     * @details This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @see BBZVM_INSTR_NEQ
     * @param[in,out] vm The VM.
     * @return The updated state of the VM.
     */
    bbzvm_state bbzvm_neq(bbzvm_t* vm);

    /**
     * @brief Performs an subtraction.
     * @details This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @see BBZVM_INSTR_GT
     * @param[in,out] vm The VM.
     * @return The updated state of the VM.
     */
    bbzvm_state bbzvm_gt(bbzvm_t* vm);

    /**
     * @brief Performs an subtraction.
     * @details This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @see BBZVM_INSTR_GTE
     * @param[in,out] vm The VM.
     * @return The updated state of the VM.
     */
    bbzvm_state bbzvm_gte(bbzvm_t* vm);

    /**
     * @brief Performs an subtraction.
     * @details This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @see BBZVM_INSTR_LT
     * @param[in,out] vm The VM.
     * @return The updated state of the VM.
     */
    bbzvm_state bbzvm_lt(bbzvm_t* vm);

    /**
     * @brief Performs an subtraction.
     * @details This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @see BBZVM_INSTR_LTE
     * @param[in,out] vm The VM.
     * @return The updated state of the VM.
     */
    bbzvm_state bbzvm_lte(bbzvm_t* vm);

    /**
     * @brief Pushes the global variable located at the given stack index.
     * @details Internally checks whether the operation is valid.
     * @see BBZVM_INSTR_GLOAD
     * @param vm The VM.
     */
    bbzvm_state bbzvm_gload(bbzvm_t* vm);

    /**
     * @brief Stores the object located at the stack top into the a global variable, pops operand.
     * @details Internally checks whether the operation is valid.
     * @see BBZVM_INSTR_GSTORE
     * @param vm The VM.
     * @param idx The local variable index.
     */
    bbzvm_state bbzvm_gstore(bbzvm_t* vm);


    /**
     * @brief Pushes a table object on the stack
     * @details Internally checks whether the operation is valid.
     * 
     * This function is designed to be used within int-returning functions such as
     * Internally checks whether the operation is valid.
     * @see BBZVM_INSTR_PUSHT
     * @param vm The VM.
     * @param idx The local variable index.
     */
    bbzvm_state bbzvm_pusht(bbzvm_t* vm);

    /**
     * @brief Stores a (idx,value) pair in a table.
     * @details Internally checks whether the operation is valid.
     * 
     * This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or bbzvm_step().
     * 
     * The stack is expected to be as follows:
     * #0 value
     * #1 idx
     * #2 table
     * This operation pops #0 and #1, leaving the table at the stack top.
     * @see BBZVM_INSTR_TPUT
     * @param vm The VM.
     */
    bbzvm_state bbzvm_tput(bbzvm_t* vm);
    
    /**
     * @brief Fetches a (idx,value) pair from a table.
     * @details Internally checks whether the operation is valid.
     * 
     * This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or bbzvm_step().
     * 
     * The stack is expected to be as follows:
     * #0 idx
     * #1 table
     * This operation pops #0 and pushes the value, leaving the table at
     * stack #1. If the element for the given idx is not found, nil is
     * pushed as value.
     * @see BBZVM_INSTR_TGET
     * @param vm The VM.
     */
    bbzvm_state bbzvm_tget(bbzvm_t* vm);


    /**
     * @brief Calls a Buzz closure.
     * @details It expects the stack to be as follows:
     * #0   arg1
     * #1   arg2
     * ...
     * #N-1   argN
     * #N closure
     * This function pops all arguments.
     * @param vm The VM.
     * @param argc The number of arguments.
     * @return 0 if everything OK, a non-zero value in case of error
     */
    bbzvm_state bbzvm_closure_call(bbzvm_t* vm, uint16_t argc);

    /**
     * @brief Calls a function defined in Buzz.
     * @details It expects the stack to be as follows:
     * #0 arg1
     * #1 arg2
     * ...
     * #N-1 argN
     * This function pops all arguments.
     * @param vm The VM.
     * @param fname The function name (bbzheap_idx_t pointing to a bbzstring_t).
     * @param argc The number of arguments.
     * @return 0 if everything OK, a non-zero value in case of error
     */
    bbzvm_state bbzvm_function_call(bbzvm_t* vm, bbzheap_idx_t fname, uint32_t argc);

    /**
     * @brief Registers a function in the VM.
     * @param vm The VM.
     * @param funp The function pointer to register.
     * @return The function id.
     */
    uint16_t bbzvm_function_register(bbzvm_t* vm, bbzvm_funp funp);

    /**
     * @brief Calls a closure.
     * @details Internally checks whether the operation is valid.
     * 
     * This function expects the stack to be as follows:
     * #0   An integer for the number of closure parameters N
     * #1   Closure arg1
     * ...
     * #N Closure argN
     * #N+1 The closure
     * 
     * This function pushes a new stack and a new local variable table filled with the
     * activation record entries and the closure arguments. In addition, it leaves the stack
     * beneath as follows:
     * #0 An integer for the return address
     * @param vm The VM.
     * @param isswrm 0 for a normal closure, 1 for a swarm closure
     * @return The VM state.
     */
    bbzvm_state bbzvm_call(bbzvm_t* vm, int isswrm);

    /**
     * @brief Calls a normal closure.
     * @details Internally checks whether the operation is valid.
     * 
     * This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * 
     * This function expects the stack to be as follows:
     * #1   An integer for the number of closure parameters N
     * #2   Closure arg1
     * ...
     * #1+N Closure argN
     * #2+N The closure
     * 
     * This function pushes a new stack and a new local variable table filled with the
     * activation record entries and the closure arguments. In addition, it leaves the stack
     * beneath as follows:
     * #1 An integer for the return address
     * @see BBZVM_INSTR_CALLC
     * @param[in,out] vm The VM.
     * @return The updated VM state.
     */
     __attribute__((always_inline)) inline
    bbzvm_state bbzvm_callc(bbzvm_t* vm) {
        return bbzvm_call(vm, 0);
    }

    /**
     * @brief Calls a swarm closure.
     * @details Internally checks whether the operation is valid.
     * 
     * This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * 
     * This function expects the stack to be as follows:
     * #1   An integer for the number of closure parameters N
     * #2   Closure arg1
     * ...
     * #1+N Closure argN
     * #2+N The closure
     * 
     * This function pushes a new stack and a new local variable table filled with the
     * activation record entries and the closure arguments. In addition, it leaves the stack
     * beneath as follows:
     * #1 An integer for the return address
     * @see BBZVM_INSTR_CALLS
     * @param[in,out] vm The VM.
     * @return The updated VM state.
     */
     __attribute__((always_inline)) inline
    bbzvm_state bbzvm_calls(bbzvm_t* vm) {
        return bbzvm_call(vm, 1);
    }

    /**
     * @brief Pushes a variable on the stack.
     * @param vm The VM.
     * @param v The variable.
     * @return The VM state.
     */
    bbzvm_state bbzvm_push(bbzvm_t* vm, bbzheap_idx_t v);

    /**
     * @brief Pushes a float value on the stack.
     * @see BBZVM_INSTR_PUSHF
     * @param vm The VM.
     * @param v The value.
     * @return The VM state.
     */
    bbzvm_state bbzvm_pushf(bbzvm_t* vm, float v);

    /**
     * @brief Pushes a 32 bit signed int value on the stack.
     * @see BBZVM_INSTR_PUSHI
     * @param vm The VM.
     * @param v The value.
     * @return The VM state.
     */
    bbzvm_state bbzvm_pushi(bbzvm_t* vm, int16_t v);

    /**
     * @brief Pushes a string on the stack.
     * @see BBZVM_INSTR_PUSHS
     * @param vm The VM.
     * @param strid The string id.
     * @return The VM state.
     */
    bbzvm_state bbzvm_pushs(bbzvm_t* vm, uint16_t strid);

    /**
     * @brief Pushes a closure on the stack.
     * @details Internally checks whether the operation is valid.
     * 
     * This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or bbzvm_step().
     * @param vm The VM.
     * @param rfrnc The closure reference.
     * @param nat 1 if the closure in native, 0 if not
     * @return The VM state.
     */
    bbzvm_state bbzvm_pushc(bbzvm_t* vm, int16_t rfrnc, int16_t nat);

    /**
     * @brief Pushes a native closure on the stack.
     * @details Internally checks whether the operation is valid.
     * 
     * This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or bbzvm_step().
     * @see BBZVM_INSTR_PUSHCN
     * @param[in,out] vm The VM.
     * @param[in] addr The closure address.
     * @return The VM state.
     */
    __attribute__((always_inline)) inline
    bbzvm_state bbzvm_pushcn(bbzvm_t* vm, int32_t addr) {
        return bbzvm_pushc(vm, addr, 1);
    }

    /**
     * @brief Pushes a c-function closure on the stack.
     * @details Internally checks whether the operation is valid.
     * 
     * This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * 
     * This function expects the function id in the stack top.
     * It pops the function id and pushes the c-function closure.
     * @see BBZVM_INSTR_PUSHCC
     * @param vm The VM data.
     * @param cid The closure id.
     * @return The updated VM state.
     */
    __attribute__((always_inline)) inline
    bbzvm_state bbzvm_pushcc(bbzvm_t* vm, int32_t cid) {
        return bbzvm_pushc(vm, cid, 0);
    }

    /**
     * @brief Pushes a lambda native closure on the stack.
     * @details Internally checks whether the operation is valid.
     * 
     * This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or bbzvm_step().
     * @see BBZVM_INSTR_PUSHL
     * @param vm The VM.
     * @param addr The closure address.
     * @return The VM state.
     */
    bbzvm_state bbzvm_pushl(bbzvm_t* vm, int16_t addr);

    /**
     * @brief Pushes a userdata on the stack.
     * @param vm The VM.
     * @param v The C pointer to the user data.
     * @return The VM state.
     */
    bbzvm_state bbzvm_pushu(bbzvm_t* vm, void* v);

    /**
     * @brief Pushes the local variable located at the given stack index.
     * @details Internally checks whether the operation is valid.
     * 
     * This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @see BBZVM_INSTR_LLOAD
     * @param[in,out] vm The VM data.
     * @param[in] idx The index of the local variable to load.
     * @return The updated VM state.
     */
    bbzvm_state bbzvm_lload(bbzvm_t* vm, uint16_t idx);

    /**
     * @brief Stores the object located at the stack top into the a local variable, pops operand.
     * @details Internally checks whether the operation is valid.
     * 
     * This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @see BBZVM_INSTR_LSTORE
     * @param[in,out] vm The VM data.
     * @param[in] idx The local variable index.
     * @return The updated VM state.
     */
    bbzvm_state bbzvm_lstore(bbzvm_t* vm, uint16_t idx);

    /**
     * @brief Sets the program counter to jump to a specificed bytecode offset.
     * @details This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @see BBZVM_INSTR_JUMP
     * @param[in,out] vm The VM.
     * @param[in] offset The offset to jump to.
     * @return The updated VM state.
     */
    bbzvm_state bbzvm_jump(bbzvm_t* vm, uint16_t offset);

    /**
     * @brief Sets the program counter to jump to a specificed bytecode
     * offset, if stack top is zero.
     * @details This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @note A nil object is considered zero.
     * @see BBZVM_INSTR_JUMPZ
     * @param[in,out] vm The VM.
     * @param[in] offset The offset to jump to.
     * @return The updated VM state.
     */
    bbzvm_state bbzvm_jumpz(bbzvm_t* vm, uint16_t offset);

    /**
     * @brief Sets the program counter to jump to a specificed bytecode
     * offset, if stack top is not zero.
     * @details This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * @note A nil object is considered zero.
     * @see BBZVM_INSTR_JUMPNZ
     * @param[in,out] vm The VM.
     * @param[in] offset The offset to jump to.
     * @return The updated VM state.
     */
    bbzvm_state bbzvm_jumpnz(bbzvm_t* vm, uint16_t offset);



    // ======================================
    // =      MISC FUNCTIONS AND MACROS     =
    // ======================================

    /**
     * @brief Fetches the object at the given index in the VM's heap.
     * @param[in] vm The VM.
     * @param[in] idx Index of the object on the VM's heap.
     * @return A pointer to the fetched object.
     */
    __attribute__((always_inline)) inline
    bbzobj_t* bbzvm_obj_at(const bbzvm_t* vm, bbzheap_idx_t idx) {
        return bbzheap_obj_at(&vm->heap, idx);
    }

    /**
     * Returns the size of the stack.
     * The most recently pushed element in the stack is at size - 1.
     * @param vm The VM.
     * @return The size of the VM's current stack.
     */
     __attribute__((always_inline)) inline
    uint16_t bbzvm_stack_size(bbzvm_t* vm) {
        return vm->stackptr + 1;
    }

    /**
     * @brief Returns the heap index of the element at given stack position,
     * where 0 is the top of the stack and >0 goes down the stack.
     * @warning This function performs no sanity check on the passed index.
     * @param[in,out] vm The VM.
     * @param[in] idx The stack index.
     * @return The heap index of the element at given stack index.
     */
    __attribute__((always_inline)) inline
    bbzheap_idx_t bbzvm_stack_at(bbzvm_t* vm, uint16_t idx) {
        return vm->stack[vm->stackptr - idx];
    }


    /**
     * Checks whether the given stack's size is >= to the passed size.
     * If the size is not valid, it updates the VM state.
     * This macro is designed to be used within int-returning functions such as
     * BuzzVM hook functions or bbzvm_step().
     * @param vm The VM.
     * @param size The stack index, where 0 is the stack top and >0 goes down the stack.
     */
    #define bbzvm_stack_assert(vm, size)                                \
        if (bbzvm_stack_size(vm) < (size)) {                            \
            bbzvm_seterror(vm, BBZVM_ERROR_STACK);                      \
            return (vm)->state;                                         \
        }
    
    /**
     * @brief Checks whether the type at the given stack position is correct.
     * If the type is wrong, it updates the VM state and exits the current function.
     * This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or bbzvm_step().
     * @param[in,out] vm The VM data.
     * @param[in] idx The stack index, where 0 is the stack top and >0 goes down the stack.
     * @param[in] tpe The type to check
     */
    #define bbzvm_type_assert(vm, idx, tpe)                             \
        {                                                               \
            bbzobj_t* o = bbzheap_obj_at(&vm->heap,                     \
                                         bbzvm_stack_at(vm, idx));      \
            if (bbztype(*o) != tpe                                      \
    			&& ((tpe & BBZTYPE_CLOSURE) == BBZTYPE_CLOSURE)        \
    			&& !bbztype_isclosure(*o)) {                       \
                bbzvm_seterror(vm, BBZVM_ERROR_TYPE);                   \
                return BBZVM_STATE_ERROR;                               \
            }                                                           \
        }

    /**
     * Calls a normal closure.
     * Internally checks whether the operation is valid.
     * This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * This function expects the stack to be as follows:
     * #1   An integer for the number of closure parameters N
     * #2   Closure arg1
     * ...
     * #1+N Closure argN
     * #2+N The closure
     * This function pushes a new stack and a new local variable table filled with the
     * activation record entries and the closure arguments. In addition, it leaves the stack
     * beneath as follows:
     * #1 An integer for the return address
     */
    #define bbzvm_callc(vm) bbzvm_call(vm, 0)

    /**
     * Calls a swarm closure.
     * Internally checks whether the operation is valid.
     * This function is designed to be used within int-returning functions such as
     * BuzzVM hook functions or buzzvm_step().
     * This function expects the stack to be as follows:
     * #1   An integer for the number of closure parameters N
     * #2   Closure arg1
     * ...
     * #1+N Closure argN
     * #2+N The closure
     * This function pushes a new stack and a new local variable table filled with the
     * activation record entries and the closure arguments. In addition, it leaves the stack
     * beneath as follows:
     * #1 An integer for the return address
     */
    #define bbzvm_calls(vm) bbzvm_call(vm, 1)


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !BBZVM_H
