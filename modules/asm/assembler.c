#include "assembler.h"
#include "../../libs/print.h"
#include "../../libs/string.h"
#include "../../drivers/serial/serial.h"
#include <stddef.h>

// Пример ASM кода для Hello World (исправленный)
const char *hello_world_asm = 
    "mov ah, 0x0E\n"           // Функция BIOS вывода символа
    "mov al, 'H'\n"
    "int 0x10\n"
    "mov al, 'e'\n"
    "int 0x10\n"
    "mov al, 'l'\n"
    "int 0x10\n"
    "mov al, 'l'\n"
    "int 0x10\n"
    "mov al, 'o'\n"
    "int 0x10\n"
    "mov al, ','\n"
    "int 0x10\n"
    "mov al, ' '\n"
    "int 0x10\n"
    "mov al, 'W'\n"
    "int 0x10\n"
    "mov al, 'o'\n"
    "int 0x10\n"
    "mov al, 'r'\n"
    "int 0x10\n"
    "mov al, 'l'\n"
    "int 0x10\n"
    "mov al, 'd'\n"
    "int 0x10\n"
    "mov al, '!'\n"
    "int 0x10\n"
    "ret";

const char *beep_asm =
    "mov al, 182\n"
    "out 43, al\n"
    "mov ax, 1193\n"
    "out 42, al\n"
    "mov al, ah\n"
    "out 42, al\n"
    "in al, 97\n"
    "or al, 3\n"
    "out 97, al\n"
    "mov cx, 65535\n"
    "delay1:\n"
    "dec cx\n"
    "jnz delay1\n"
    "in al, 97\n"
    "and al, 252\n"
    "out 97, al\n"
    "ret";

// Расширенная таблица инструкций
typedef struct {
    const char *mnemonic;
    uint8_t opcode;
    uint8_t size;
    uint8_t has_operand;
    const char *format;
} AsmInstruction;

static AsmInstruction instructions[] = {
    // 8-битные MOV
    {"mov al,", 0xB0, 2, 1, "imm8"},
    {"mov cl,", 0xB1, 2, 1, "imm8"},
    {"mov dl,", 0xB2, 2, 1, "imm8"},
    {"mov bl,", 0xB3, 2, 1, "imm8"},
    {"mov ah,", 0xB4, 2, 1, "imm8"},
    {"mov ch,", 0xB5, 2, 1, "imm8"},
    {"mov dh,", 0xB6, 2, 1, "imm8"},
    {"mov bh,", 0xB7, 2, 1, "imm8"},
    
    // 32-битные MOV
    {"mov eax,", 0xB8, 5, 1, "imm32"},
    {"mov ecx,", 0xB9, 5, 1, "imm32"},
    {"mov edx,", 0xBA, 5, 1, "imm32"},
    {"mov ebx,", 0xBB, 5, 1, "imm32"},
    {"mov esp,", 0xBC, 5, 1, "imm32"},
    {"mov ebp,", 0xBD, 5, 1, "imm32"},
    {"mov esi,", 0xBE, 5, 1, "imm32"},
    {"mov edi,", 0xBF, 5, 1, "imm32"},
    
    // Простые инструкции
    {"nop", 0x90, 1, 0, ""},
    {"ret", 0xC3, 1, 0, ""},
    {"retf", 0xCB, 1, 0, ""},
    {"hlt", 0xF4, 1, 0, ""},
    {"cli", 0xFA, 1, 0, ""},
    {"sti", 0xFB, 1, 0, ""},
    {"cld", 0xFC, 1, 0, ""},
    {"std", 0xFD, 1, 0, ""},
    
    // Арифметические
    {"inc eax", 0x40, 1, 0, ""},
    {"inc ecx", 0x41, 1, 0, ""},
    {"inc edx", 0x42, 1, 0, ""},
    {"inc ebx", 0x43, 1, 0, ""},
    {"inc esp", 0x44, 1, 0, ""},
    {"inc ebp", 0x45, 1, 0, ""},
    {"inc esi", 0x46, 1, 0, ""},
    {"inc edi", 0x47, 1, 0, ""},
    
    {"dec eax", 0x48, 1, 0, ""},
    {"dec ecx", 0x49, 1, 0, ""},
    {"dec edx", 0x4A, 1, 0, ""},
    {"dec ebx", 0x4B, 1, 0, ""},
    {"dec esp", 0x4C, 1, 0, ""},
    {"dec ebp", 0x4D, 1, 0, ""},
    {"dec esi", 0x4E, 1, 0, ""},
    {"dec edi", 0x4F, 1, 0, ""},
    
    // ADD
    {"add al,", 0x04, 2, 1, "imm8"},
    {"add cl,", 0x80, 3, 1, "imm8_c1"},
    {"add dl,", 0x80, 3, 1, "imm8_c2"},
    {"add bl,", 0x80, 3, 1, "imm8_c3"},
    
    // SUB
    {"sub al,", 0x2C, 2, 1, "imm8"},
    {"sub cl,", 0x80, 3, 1, "imm8_c5"},
    {"sub dl,", 0x80, 3, 1, "imm8_c6"},
    {"sub bl,", 0x80, 3, 1, "imm8_c7"},
    
    // CMP
    {"cmp al,", 0x3C, 2, 1, "imm8"},
    
    // Потоковые инструкции
    {"jmp", 0xEB, 2, 1, "rel8"},
    {"je", 0x74, 2, 1, "rel8"},
    {"jne", 0x75, 2, 1, "rel8"},
    {"jl", 0x7C, 2, 1, "rel8"},
    {"jle", 0x7E, 2, 1, "rel8"},
    {"jg", 0x7F, 2, 1, "rel8"},
    {"jge", 0x7D, 2, 1, "rel8"},
    {"jb", 0x72, 2, 1, "rel8"},
    {"jbe", 0x76, 2, 1, "rel8"},
    {"ja", 0x77, 2, 1, "rel8"},
    {"jae", 0x73, 2, 1, "rel8"},
    {"jcxz", 0xE3, 2, 1, "rel8"},
    {"jecxz", 0xE3, 2, 1, "rel8"},
    {"loop", 0xE2, 2, 1, "rel8"},
    
    // INT
    {"int", 0xCD, 2, 1, "imm8"},
    
    // IN/OUT
    {"in al,", 0xE4, 2, 1, "port8"},
    {"out", 0xE6, 2, 1, "port8_al"},
    {"out 43, al", 0xE6, 2, 0, ""},
    {"out 42, al", 0xE6, 2, 0, ""},
    {"out 97, al", 0xE6, 2, 0, ""},
    {"in al, 97", 0xE4, 2, 0, ""},
    {"in al, 43", 0xE4, 2, 0, ""},
    {"in al, 42", 0xE4, 2, 0, ""},
    
    // Логические
    {"or al,", 0x0C, 2, 1, "imm8"},
    {"and al,", 0x24, 2, 1, "imm8"},
    {"xor al,", 0x34, 2, 1, "imm8"},
    {"test al,", 0xA8, 2, 1, "imm8"},
    
    // PUSH/POP
    {"push eax", 0x50, 1, 0, ""},
    {"push ecx", 0x51, 1, 0, ""},
    {"push edx", 0x52, 1, 0, ""},
    {"push ebx", 0x53, 1, 0, ""},
    {"push esp", 0x54, 1, 0, ""},
    {"push ebp", 0x55, 1, 0, ""},
    {"push esi", 0x56, 1, 0, ""},
    {"push edi", 0x57, 1, 0, ""},
    
    {"pop eax", 0x58, 1, 0, ""},
    {"pop ecx", 0x59, 1, 0, ""},
    {"pop edx", 0x5A, 1, 0, ""},
    {"pop ebx", 0x5B, 1, 0, ""},
    {"pop esp", 0x5C, 1, 0, ""},
    {"pop ebp", 0x5D, 1, 0, ""},
    {"pop esi", 0x5E, 1, 0, ""},
    {"pop edi", 0x5F, 1, 0, ""},
    
    // MUL/IMUL
    {"mul cl", 0xF6, 2, 0, "mul_c1"},
    {"mul dl", 0xF6, 2, 0, "mul_c2"},
    {"mul bl", 0xF6, 2, 0, "mul_c3"},
    
    // DIV/IDIV
    {"div cl", 0xF6, 2, 0, "div_c1"},
    {"div dl", 0xF6, 2, 0, "div_c2"},
    {"div bl", 0xF6, 2, 0, "div_c3"},
};

#define INSTRUCTION_COUNT (sizeof(instructions) / sizeof(instructions[0]))

// Парсер чисел
static bool parse_number(const char *str, uint32_t *value) {
    if (str[0] == '0' && str[1] == 'x') {
        // Шестнадцатеричное
        *value = 0;
        for (int i = 2; str[i]; i++) {
            char c = str[i];
            *value <<= 4;
            if (c >= '0' && c <= '9') *value |= c - '0';
            else if (c >= 'a' && c <= 'f') *value |= c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') *value |= c - 'A' + 10;
            else return false;
        }
        return true;
    } else if (str[0] == '\'') {
        // Символ
        if (str[1] && str[2] == '\'') {
            *value = str[1];
            return true;
        }
        return false;
    } else {
        // Десятичное
        *value = 0;
        for (int i = 0; str[i]; i++) {
            if (str[i] >= '0' && str[i] <= '9') {
                *value = *value * 10 + (str[i] - '0');
            } else {
                return false;
            }
        }
        return true;
    }
}

// Извлечение операнда из строки
static char* extract_operand(const char *line, const char *mnemonic) {
    static char operand[64];
    
    // Находим позицию после мнемоники
    const char *pos = line + strlen(mnemonic);
    
    // Пропускаем пробелы
    while (*pos == ' ' || *pos == '\t') pos++;
    
    // Копируем до конца строки или до комментария
    int i = 0;
    while (*pos && *pos != ';' && i < 63) {
        operand[i++] = *pos++;
    }
    operand[i] = '\0';
    
    return operand;
}

// Основная функция ассемблирования
bool assemble_x86(const char *source, uint8_t *output, uint32_t max_size, uint32_t *out_size) {
    char buffer[MAX_ASM_CODE];
    strcpy(buffer, source);
    
    uint32_t pos = 0;
    char *line = strtok(buffer, "\n");
    int line_num = 1;
    
    while (line && pos < max_size - 16) {
        // Очистка строки
        while (*line == ' ' || *line == '\t') line++;
        
        // Удаление комментариев
        char *comment = strchr(line, ';');
        if (comment) *comment = '\0';
        
        // Обрезка пробелов в конце
        int len = strlen(line);
        while (len > 0 && (line[len-1] == ' ' || line[len-1] == '\t')) {
            line[--len] = '\0';
        }
        
        // Пропуск пустых строк
        if (len == 0) {
            line = strtok(NULL, "\n");
            line_num++;
            continue;
        }
        
        // Поиск инструкции
        bool found = false;
        for (int i = 0; i < INSTRUCTION_COUNT; i++) {
            if (strncmp(line, instructions[i].mnemonic, strlen(instructions[i].mnemonic)) == 0) {
                found = true;
                
                // Обработка в зависимости от формата
                if (strcmp(instructions[i].format, "imm8") == 0) {
                    char *operand = extract_operand(line, instructions[i].mnemonic);
                    uint32_t value;
                    
                    if (parse_number(operand, &value)) {
                        output[pos++] = instructions[i].opcode;
                        output[pos++] = value & 0xFF;
                    } else {
                        print("Error: Invalid operand '", RED);
                        print(operand, WHITE);
                        print("' at line ", RED);
                        print_dec(line_num, WHITE);
                        print("\n", WHITE);
                        return false;
                    }
                }
                else if (strcmp(instructions[i].format, "imm32") == 0) {
                    char *operand = extract_operand(line, instructions[i].mnemonic);
                    uint32_t value;
                    
                    if (parse_number(operand, &value)) {
                        output[pos++] = instructions[i].opcode;
                        output[pos++] = value & 0xFF;
                        output[pos++] = (value >> 8) & 0xFF;
                        output[pos++] = (value >> 16) & 0xFF;
                        output[pos++] = (value >> 24) & 0xFF;
                    } else {
                        print("Error: Invalid operand '", RED);
                        print(operand, WHITE);
                        print("' at line ", RED);
                        print_dec(line_num, WHITE);
                        print("\n", WHITE);
                        return false;
                    }
                }
                else if (strcmp(instructions[i].format, "imm8_c1") == 0) {
                    // ADD CL, imm8
                    char *operand = extract_operand(line, instructions[i].mnemonic);
                    uint32_t value;
                    
                    if (parse_number(operand, &value)) {
                        output[pos++] = instructions[i].opcode; // 0x80
                        output[pos++] = 0xC1; // ADD CL
                        output[pos++] = value & 0xFF;
                    } else {
                        print("Error: Invalid operand\n", RED);
                        return false;
                    }
                }
                else if (strcmp(instructions[i].format, "rel8") == 0) {
                    // JMP rel8
                    char *operand = extract_operand(line, instructions[i].mnemonic);
                    uint32_t value;
                    
                    if (parse_number(operand, &value)) {
                        output[pos++] = instructions[i].opcode;
                        output[pos++] = value & 0xFF;
                    } else {
                        print("Error: Invalid operand\n", RED);
                        return false;
                    }
                }
                else if (strcmp(instructions[i].format, "port8") == 0) {
                    // IN AL, port8
                    char *operand = extract_operand(line, instructions[i].mnemonic);
                    uint32_t value;
                    
                    if (parse_number(operand, &value)) {
                        output[pos++] = instructions[i].opcode;
                        output[pos++] = value & 0xFF;
                    } else {
                        print("Error: Invalid port\n", RED);
                        return false;
                    }
                }
                else if (strcmp(instructions[i].format, "port8_al") == 0) {
                    // OUT port8, AL
                    char *operand = extract_operand(line, "");
                    uint32_t value;
                    
                    if (parse_number(operand, &value)) {
                        output[pos++] = instructions[i].opcode;
                        output[pos++] = value & 0xFF;
                    } else {
                        print("Error: Invalid port\n", RED);
                        return false;
                    }
                }
                else if (instructions[i].has_operand == 0) {
                    // Инструкция без операндов
                    output[pos++] = instructions[i].opcode;
                }
                else {
                    // INT и другие с одним операндом
                    char *operand = extract_operand(line, instructions[i].mnemonic);
                    uint32_t value;
                    
                    if (parse_number(operand, &value)) {
                        output[pos++] = instructions[i].opcode;
                        output[pos++] = value & 0xFF;
                    } else {
                        print("Error: Invalid operand\n", RED);
                        return false;
                    }
                }
                
                break;
            }
        }
        
        if (!found) {
            // Проверяем специальные случаи
            if (strncmp(line, "out ", 4) == 0) {
                // Обработка OUT port, al
                char port_str[10];
                uint32_t port;
                
                if (sscanf(line + 4, "%s", port_str) == 1) {
                    if (parse_number(port_str, &port)) {
                        output[pos++] = 0xE6;
                        output[pos++] = port & 0xFF;
                    } else {
                        goto unknown_instruction;
                    }
                } else {
                    goto unknown_instruction;
                }
            }
            else if (strncmp(line, "in al, ", 7) == 0) {
                // Обработка IN AL, port
                char port_str[10];
                uint32_t port;
                
                if (sscanf(line + 7, "%s", port_str) == 1) {
                    if (parse_number(port_str, &port)) {
                        output[pos++] = 0xE4;
                        output[pos++] = port & 0xFF;
                    } else {
                        goto unknown_instruction;
                    }
                } else {
                    goto unknown_instruction;
                }
            }
            else {
                unknown_instruction:
                print("Unknown instruction at line ", RED);
                print_dec(line_num, WHITE);
                print(": ", RED);
                print(line, WHITE);
                print("\n", WHITE);
                return false;
            }
        }
        
        line = strtok(NULL, "\n");
        line_num++;
    }
    
    *out_size = pos;
    return true;
}

// Функция выполнения ASM кода
// Функция выполнения ASM кода
bool execute_asm(const char *source) {
    uint8_t machine_code[MAX_MACHINE_CODE];
    uint32_t code_size;
    
    print("Assembling code...\n", CYAN);
    
    if (!assemble_x86(source, machine_code, MAX_MACHINE_CODE, &code_size)) {
        print_error("Assembly failed");
        return false;
    }
    
    print("Assembly successful. Code size: ", GREEN);
    print_dec(code_size, CYAN);
    print(" bytes\n", GREEN);
    
    // Покажем машинный код
    print("Machine code:\n", WHITE);
    for (uint32_t i = 0; i < code_size; i++) {
        if (i % 8 == 0) {
            print("  ", WHITE);
            print_hex(i, GRAY);
            print(": ", WHITE);
        }
        print_hex(machine_code[i], CYAN);
        print(" ", WHITE);
        if (i % 8 == 7 || i == code_size - 1) print("\n", WHITE);
    }
    
    // Выделяем исполняемую память
    static uint8_t exec_buffer[1024] __attribute__((aligned(16)));
    if (code_size > sizeof(exec_buffer)) {
        print_error("Code too large");
        return false;
    }
    
    // Копируем код в исполняемый буфер
    memcpy(exec_buffer, machine_code, code_size);
    
    print("Executing at 0x", CYAN);
    print_hex((uint32_t)exec_buffer, WHITE);
    print("...\n", CYAN);
    
    // Безопасное выполнение с сохранением и восстановлением регистров
    uint32_t eax_result;
    
    __asm__ volatile (
        // Сохраняем все регистры
        "pushal\n"
        // Сохраняем флаги
        "pushfl\n"
        // Вызываем наш код
        "call *%1\n"
        // Сохраняем результат из EAX
        "movl %%eax, %0\n"
        // Восстанавливаем флаги
        "popfl\n"
        // Восстанавливаем регистры
        "popal\n"
        : "=r" (eax_result)          // Выходной параметр
        : "r" (exec_buffer)          // Входной параметр
        : "memory", "cc"             // Изменяет память и флаги
    );
    
    print("Execution completed. EAX = 0x", GREEN);
    print_hex(eax_result, CYAN);
    print("\n", GREEN);
    
    return true;
}

// Демо: Hello World
void asm_demo_hello_world(void) {
    print("=== ASM Hello World Demo ===\n", CYAN);
    if (execute_asm(hello_world_asm)) {
        print("Hello World executed successfully!\n", GREEN);
    }
}

// Демо: звуковой сигнал
void asm_demo_beep(void) {
    print("=== ASM Beep Demo ===\n", CYAN);
    if (execute_asm(beep_asm)) {
        print("Beep executed successfully!\n", GREEN);
    }
}

// Демо: работа с регистрами
void asm_demo_registers(void) {
    const char *reg_test = 
        "mov eax, 0x12345678\n"
        "mov ebx, 0x87654321\n"
        "add eax, ebx\n"
        "xor edx, edx\n"
        "mov ecx, 10\n"
        "loop_start:\n"
        "inc edx\n"
        "loop loop_start\n"
        "ret";
    
    print("=== ASM Registers Test ===\n", CYAN);
    execute_asm(reg_test);
}

// Простой калькулятор на ассемблере
void asm_demo_calculator(void) {
    const char *calc_asm = 
        "mov eax, 10\n"
        "mov ebx, 20\n"
        "add eax, ebx\n"      // 10 + 20 = 30
        "sub eax, 5\n"        // 30 - 5 = 25
        "mov ecx, 5\n"
        "mul ecx\n"           // 25 * 5 = 125
        "mov edx, 0\n"
        "mov ecx, 25\n"
        "div ecx\n"           // 125 / 25 = 5
        "ret";
    
    print("=== ASM Calculator Demo ===\n", CYAN);
    execute_asm(calc_asm);
}