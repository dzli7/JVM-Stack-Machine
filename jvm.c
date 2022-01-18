#include "jvm.h"

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "heap.h"
#include "read_class.h"

/** The name of the method to invoke to run the class file */
const char MAIN_METHOD[] = "main";
/**
 * The "descriptor" string for main(). The descriptor encodes main()'s signature,
 * i.e. main() takes a String[] and returns void.
 * If you're interested, the descriptor string is explained at
 * https://docs.oracle.com/javase/specs/jvms/se12/html/jvms-4.html#jvms-4.3.2.
 */
const char MAIN_DESCRIPTOR[] = "([Ljava/lang/String;)V";

/**
 * Represents the return value of a Java method: either void or an int or a reference.
 * For simplification, we represent a reference as an index into a heap-allocated array.
 * (In a real JVM, methods could also return object references or other primitives.)
 */
typedef struct {
    /** Whether this returned value is an int */
    bool has_value;
    /** The returned value (only valid if `has_value` is true) */
    int32_t value;
} optional_value_t;
int level;
/**
 * Runs a method's instructions until the method returns.
 *
 * @param method the method to run
 * @param locals the array of local variables, including the method parameters.
 *   Except for parameters, the locals are uninitialized.
 * @param class the class file the method belongs to
 * @param heap an array of heap-allocated pointers, useful for references
 * @return an optional int containing the method's return value
 */
optional_value_t execute(method_t *method, int32_t *locals, class_file_t *class,
                         heap_t *heap) {
    /* You should remove these casts to void in your solution.
     * They are just here so the code compiles without warnings. */
    size_t pc = 0;
    size_t stack_index = 0;
    code_t method_code = method->code;
    u1 *code = method_code.code;
    int32_t *stack = calloc(method_code.max_stack, sizeof(int32_t));
    while (pc < method_code.code_length) {
        u1 instruction = code[pc];

        if (instruction == i_return) {
            break;
        }
        // task 8
        else if (instruction == i_ireturn) {
            optional_value_t result = {.has_value = true,
                                       .value = stack[stack_index - 1]};
            stack_index--;
            pc++;
            free(stack);
            return result;
        }
        else if (instruction == i_invokestatic) {
            int16_t pool_index = (int16_t)(code[pc + 1] << 8) | code[pc + 2];
            method_t *m = find_method_from_index(pool_index, class);
            int16_t parameters = (int16_t) get_number_of_parameters(m);
            int32_t *local_temp = calloc(m->code.max_locals, sizeof(int32_t));
            for (int16_t i = 0; i < parameters; i++) {
                local_temp[parameters - i - 1] = (int32_t) stack[stack_index - i - 1];
            }
            stack_index -= parameters;
            optional_value_t val = execute(m, local_temp, class, heap);
            if (val.has_value) {
                stack[stack_index] = val.value;
                stack_index++;
            }
            pc += 3;
            free(local_temp);
        }
        else if (instruction == i_bipush) {
            stack[stack_index] = (int) ((int8_t) code[pc + 1]);
            pc += 2;
            stack_index++;
        }
        else if (instruction == i_iadd) {
            assert(stack_index >= 2);
            int32_t c = stack[stack_index - 2] + stack[stack_index - 1];
            stack[stack_index - 2] = c;
            stack_index--;
            pc++;
        }
        else if (instruction == i_isub) {
            assert(stack_index >= 2);
            int32_t c = stack[stack_index - 2] - stack[stack_index - 1];
            stack[stack_index - 2] = c;
            stack_index--;
            pc++;
        }
        else if (instruction == i_imul) {
            assert(stack_index >= 2);
            int32_t c = stack[stack_index - 2] * stack[stack_index - 1];
            stack[stack_index - 2] = c;
            stack_index--;
            pc++;
        }
        else if (instruction == i_idiv) {
            assert(stack_index >= 2);
            assert(stack[stack_index - 1] != 0);
            int32_t c = stack[stack_index - 2] / stack[stack_index - 1];
            stack[stack_index - 2] = c;
            stack_index--;
            pc++;
        }
        else if (instruction == i_irem) {
            assert(stack_index >= 2);
            assert(stack[stack_index - 1] != 0);
            int32_t c = stack[stack_index - 2] % stack[stack_index - 1];
            stack[stack_index - 2] = c;
            stack_index--;
            pc++;
        }
        else if (instruction == i_ineg) {
            stack[stack_index - 1] *= -1;
            pc++;
        }
        else if (instruction == i_ishl) {
            assert(stack_index >= 2);
            int32_t c = stack[stack_index - 2] << stack[stack_index - 1];
            stack[stack_index - 2] = c;
            stack_index--;
            pc++;
        }
        else if (instruction == i_ishr) {
            assert(stack_index >= 2);
            int32_t c = stack[stack_index - 2] >> stack[stack_index - 1];
            stack[stack_index - 2] = c;
            stack_index--;
            pc++;
        }
        else if (instruction == i_iushr) {
            assert(stack_index >= 2);
            int32_t c = ((uint32_t) stack[stack_index - 2]) >> stack[stack_index - 1];
            stack[stack_index - 2] = c;
            stack_index--;
            pc++;
        }
        else if (instruction == i_iand) {
            assert(stack_index >= 2);
            int32_t c = stack[stack_index - 2] & stack[stack_index - 1];
            stack[stack_index - 2] = c;
            stack_index--;
            pc++;
        }
        else if (instruction == i_ior) {
            assert(stack_index >= 2);
            int32_t c = stack[stack_index - 2] | stack[stack_index - 1];
            stack[stack_index - 2] = c;
            stack_index--;
            pc++;
        }
        else if (instruction == i_ixor) {
            assert(stack_index >= 2);
            int32_t c = stack[stack_index - 2] ^ stack[stack_index - 1];
            stack[stack_index - 2] = c;
            stack_index--;
            pc++;
        }
        else if (instruction == i_getstatic) {
            pc += 3;
        }
        else if (instruction == i_invokevirtual) {
            assert(stack_index > 0);
            stack_index--;
            printf("%d\n", stack[stack_index]);
            pc += 3;
        }
        else if (instruction >= i_iconst_m1 && instruction <= i_iconst_5) {
            stack[stack_index] = instruction - i_iconst_0;
            stack_index++;
            pc++;
        }
        else if (instruction == i_sipush) {
            int16_t b_concat = ((code[pc + 1] << 8) | code[pc + 2]);
            stack[stack_index] = b_concat;
            pc += 3;
            stack_index++;
        }
        else if (instruction == i_iload) {
            stack[stack_index] = locals[code[pc + 1]];
            pc += 2;
            stack_index++;
        }
        else if (instruction == i_istore) {
            locals[code[pc + 1]] = stack[stack_index - 1];
            stack_index--;
            pc += 2;
        }
        else if (instruction == i_iinc) {
            locals[code[pc + 1]] += (int8_t) code[pc + 2];
            pc += 3;
        }
        else if (instruction >= i_iload_0 && instruction <= i_iload_3) {
            stack[stack_index] = locals[instruction - i_iload_0];
            stack_index++;
            pc++;
        }
        else if (instruction >= i_istore_0 && instruction <= i_istore_3) {
            locals[instruction - i_istore_0] = stack[stack_index - 1];
            stack_index--;
            pc++;
        }
        // task 6
        else if (instruction == i_ldc) {
            stack[stack_index] =
                *((int32_t *) class->constant_pool[(int32_t) code[pc + 1] - 1].info);
            stack_index++;
            pc += 2;
        }
        // task 7
        else if (instruction == i_ifeq) {
            assert(stack_index > 0);
            if (stack[stack_index - 1] == 0) {
                pc += (size_t)((int16_t)(code[pc + 1] << 8) | code[pc + 2]);
            }
            else {
                pc += 3;
            }
            stack_index--;
        }
        else if (instruction == i_ifne) {
            assert(stack_index > 0);
            if (stack[stack_index - 1] != 0) {
                pc += (size_t)((int16_t)(code[pc + 1] << 8) | code[pc + 2]);
            }
            else {
                pc += 3;
            }
            stack_index--;
        }
        else if (instruction == i_iflt) {
            assert(stack_index > 0);
            if (stack[stack_index - 1] < 0) {
                pc += (size_t)((int16_t)(code[pc + 1] << 8) | code[pc + 2]);
            }
            else {
                pc += 3;
            }
            stack_index--;
        }
        else if (instruction == i_ifge) {
            assert(stack_index > 0);
            if (stack[stack_index - 1] >= 0) {
                pc += (size_t)((int16_t)(code[pc + 1] << 8) | code[pc + 2]);
            }
            else {
                pc += 3;
            }
            stack_index--;
        }
        else if (instruction == i_ifgt) {
            assert(stack_index > 0);
            if (stack[stack_index - 1] > 0) {
                pc += (size_t)((int16_t)(code[pc + 1] << 8) | code[pc + 2]);
            }
            else {
                pc += 3;
            }
            stack_index--;
        }
        else if (instruction == i_ifle) {
            assert(stack_index > 0);
            if (stack[stack_index - 1] <= 0) {
                pc += (size_t)((int16_t)(code[pc + 1] << 8) | code[pc + 2]);
            }
            else {
                pc += 3;
            }
            stack_index--;
        }
        else if (instruction == i_if_icmpeq) {
            assert(stack_index >= 2);
            if (stack[stack_index - 2] == stack[stack_index - 1]) {
                pc += (size_t)((int16_t)(code[pc + 1] << 8) | code[pc + 2]);
            }
            else {
                pc += 3;
            }
            stack_index -= 2;
        }
        else if (instruction == i_if_icmpne) {
            assert(stack_index >= 2);
            if (stack[stack_index - 2] != stack[stack_index - 1]) {
                pc += (size_t)((int16_t)(code[pc + 1] << 8) | code[pc + 2]);
            }
            else {
                pc += 3;
            }
            stack_index -= 2;
        }
        else if (instruction == i_if_icmplt) {
            assert(stack_index >= 2);
            if (stack[stack_index - 2] < stack[stack_index - 1]) {
                pc += (size_t)((int16_t)(code[pc + 1] << 8) | code[pc + 2]);
            }
            else {
                pc += 3;
            }
            stack_index -= 2;
        }
        else if (instruction == i_if_icmpge) {
            assert(stack_index >= 2);
            if (stack[stack_index - 2] >= stack[stack_index - 1]) {
                pc += (size_t)((int16_t)(code[pc + 1] << 8) | code[pc + 2]);
            }
            else {
                pc += 3;
            }
            stack_index -= 2;
        }
        else if (instruction == i_if_icmpgt) {
            assert(stack_index >= 2);
            if (stack[stack_index - 2] > stack[stack_index - 1]) {
                pc += (size_t)((int16_t)(code[pc + 1] << 8) | code[pc + 2]);
            }
            else {
                pc += 3;
            }
            stack_index -= 2;
        }
        else if (instruction == i_if_icmple) {
            assert(stack_index >= 2);
            if (stack[stack_index - 2] <= stack[stack_index - 1]) {
                pc += (size_t)((int16_t)(code[pc + 1] << 8) | code[pc + 2]);
            }
            else {
                pc += 3;
            }
            stack_index -= 2;
        }
        else if (instruction == i_goto) {
            pc += (size_t)((int16_t)(code[pc + 1] << 8) | code[pc + 2]);
        }
        // task 9
        else if (instruction == i_nop) {
            pc++;
        }
        else if (instruction == i_dup) {
            stack[stack_index] = stack[stack_index - 1];
            stack_index++;
            pc++;
        }
        else if (instruction == i_newarray) {
            int32_t *arr = calloc(sizeof(int32_t), (stack[stack_index - 1] + 1));
            for (int32_t i = 1; i < stack[stack_index - 1] + 1; i++) {
                arr[i] = 0;
            }
            arr[0] = stack[stack_index - 1];
            stack[stack_index - 1] = heap_add(heap, arr);
            pc += 2;
        }
        else if (instruction == i_arraylength) {
            int32_t *arr = heap_get(heap, stack[stack_index - 1]);
            stack[stack_index - 1] = arr[0];
            pc++;
        }
        else if (instruction == i_areturn) {
            optional_value_t result = {.has_value = true,
                                       .value = stack[stack_index - 1]};
            pc++;
            stack_index--;
            free(stack);
            return result;
        }
        else if (instruction == i_iastore) {
            int32_t *arr = heap_get(heap, stack[stack_index - 3]);
            arr[stack[stack_index - 2] + 1] = stack[stack_index - 1];
            stack_index -= 3;
            pc++;
        }
        else if (instruction == i_iaload) {
            int32_t *arr = heap_get(heap, stack[stack_index - 2]);
            stack[stack_index - 2] = arr[stack[stack_index - 1] + 1];
            stack_index--;
            pc++;
        }
        else if (instruction == i_aload) {
            stack[stack_index] = locals[code[pc + 1]];
            stack_index++;
            pc += 2;
        }
        else if (instruction == i_astore) {
            locals[code[pc + 1]] = stack[stack_index - 1];
            pc += 2;
            stack_index--;
        }
        else if (instruction <= i_aload_3 && instruction >= i_aload_0) {
            stack[stack_index] = locals[instruction - i_aload_0];
            pc++;
            stack_index++;
        }
        else if (instruction <= i_astore_3 && instruction >= i_astore_0) {
            locals[instruction - i_astore_0] = stack[stack_index - 1];
            pc++;
            stack_index--;
        }
    }
    free(stack);

    // Return void
    optional_value_t result = {.has_value = false};
    return result;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <class file>\n", argv[0]);
        return 1;
    }

    // Open the class file for reading
    FILE *class_file = fopen(argv[1], "r");
    assert(class_file != NULL && "Failed to open file");

    // Parse the class file
    class_file_t *class = get_class(class_file);
    int error = fclose(class_file);
    assert(error == 0 && "Failed to close file");

    // The heap array is initially allocated to hold zero elements.
    heap_t *heap = heap_init();

    // Execute the main method
    method_t *main_method = find_method(MAIN_METHOD, MAIN_DESCRIPTOR, class);
    assert(main_method != NULL && "Missing main() method");
    /* In a real JVM, locals[0] would contain a reference to String[] args.
     * But since TeenyJVM doesn't support Objects, we leave it uninitialized. */
    int32_t locals[main_method->code.max_locals];
    // Initialize all local variables to 0
    memset(locals, 0, sizeof(locals));
    optional_value_t result = execute(main_method, locals, class, heap);
    assert(!result.has_value && "main() should return void");

    // Free the internal data structures
    free_class(class);

    // Free the heap
    heap_free(heap);
}
