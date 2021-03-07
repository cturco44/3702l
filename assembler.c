/**
 * Project 1 
 * Assembler code fragment for LC-2K 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#define MAXLINELENGTH 1000

struct Label_holder {
    char label[7];
    int address;
    bool global;
};
struct Data_holder {
    int fills[100];
    int size;
};
void print_data(FILE *outFilePtr, const struct Data_holder* d) {
    for(int i = 0; i < d->size; ++i) {
        fprintf(outFilePtr, "%d\n", d->fills[i]);
    }
}
void push_back_data(int value, struct Data_holder* holder) {
    holder->fills[holder->size] = value;
    holder->size++;
}
struct Symbol {
    char symbol[7];
    char letter;
    int offset;
};
void initialize_symbol(struct Symbol* s, char* symbol_in, char letter_in, int offset_in) {
    strcpy(s->symbol, symbol_in);
    s->letter = letter_in;
    s->offset = offset_in;
}
struct Symbol_holder {
    struct Symbol fills[100];
    int size;
};
bool in_symbols(const struct Symbol_holder* s, char* str) {
    for(int i = 0; i < s->size; ++i) {
        if(strcmp(s->fills[i].symbol, str) == 0) {
            return true;
        }
    }
    return false;
}
void print_symbols(FILE *outFilePtr, const struct Symbol_holder* s) {
    for(int i = 0; i < s->size; ++i) {
        fprintf(outFilePtr, "%s %c %d\n", s->fills[i].symbol, s->fills[i].letter, s->fills[i].offset);
    }
}
void push_back_symbol(struct Symbol* s, struct Symbol_holder* holder) {
    bool in_symbols = false;
    for(int i = 0; i < holder->size; ++i) {
        if(strcmp(s->symbol, holder->fills[i].symbol) == 0) {
            in_symbols = true;
        }
    }
    if(!in_symbols) {
        holder->fills[holder->size] = *s;
        holder->size++;
    }
}
struct Relocation {
    int offset;
    char opcode[6];
    char label[7];
};
void initialize_relocation(struct Relocation* r, int offset_in, char* opcode_in, char* label_in) {
    r->offset = offset_in;
    strcpy(r->opcode, opcode_in);
    strcpy(r->label, label_in);
}
struct Relocation_holder {
    struct Relocation fills[100];
    int size;
};
void print_relocation(FILE *outFilePtr, const struct Relocation_holder* r) {
    for(int i = 0; i < r->size; ++i) {
        fprintf(outFilePtr, "%d %s %s\n", r->fills[i].offset, r->fills[i].opcode, r->fills[i].label);
    }
}
void push_back_relocation(struct Relocation* r, struct Relocation_holder* holder) {
    holder->fills[holder->size] = *r;
    holder->size++;
}

int readAndParse(FILE *, char *, char *, char *, char *, char *);
int isNumber(char *);
int itype(char* opcode, char* regA, char* regB, char* offset, struct Label_holder *label_holder, int label_idx, int pc);
int opcode_binary(char * opcode_in);
int get_label(char* reg, struct Label_holder *label_holder, int label_idx);
void print_info(FILE *outFile, int pc, int binary);
int rtype(char* opcode, char* regA, char* regB, char* dest, struct Label_holder *label_holder, int label_idx, int pc);
int jtype(char* opcode, char* regA, char* regB, struct Label_holder *label_holder, int label_idx, int pc);
int fill(char* arg0, struct Label_holder *label_holder, int label_idx);
int check_duplicate_label(char* label, struct Label_holder *label_holder, int label_idx);
bool global_defined(char* g, FILE *inFilePtr);

int
main(int argc, char *argv[])
{
    char *inFileString, *outFileString;
    FILE *inFilePtr, *outFilePtr;
    char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH],
            arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];

    if (argc != 3) {
        printf("error: usage: %s <assembly-code-file> <machine-code-file>\n",
            argv[0]);
        exit(1);
    }

    inFileString = argv[1];
    outFileString = argv[2];

    inFilePtr = fopen(inFileString, "r");
    if (inFilePtr == NULL) {
        printf("error in opening %s\n", inFileString);
        exit(1);
    }
    outFilePtr = fopen(outFileString, "w");
    if (outFilePtr == NULL) {
        printf("error in opening %s\n", outFileString);
        exit(1);
    }

    /* here is an example for how to use readAndParse to read a line from
        inFilePtr */
//    if (! readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2) ) {
//
//        /* reached end of file */
//    }
    struct Label_holder all_labels[100];
    struct Relocation_holder all_relocation;
    all_relocation.size = 0;
    struct Symbol_holder all_symbols;
    all_symbols.size = 0;
    
    int idx = 0;
    int line_no = 0;
    int text_size = 0;
    int data_size = 0;
    bool data_yet = false;
    
    while(readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)) {
        
        if(!strcmp(opcode, ".fill")) {
            ++data_size;
            data_yet = true;
        }
        else {
            ++text_size;
        }
        char relevant_label[7] = {'\0'};
        if(!strcmp(opcode, ".fill")) {
            strcpy(relevant_label, arg0);
        }
        else if (!strcmp(opcode, "lw") || !strcmp(opcode, "sw")) {
            strcpy(relevant_label, arg2);
        }
        
        //If theres a label
        if(relevant_label[0] != '\0' && !isNumber(relevant_label)) {
            //If its a global put in symbol table
            if(isupper(relevant_label[0])) {
                struct Symbol s;
                FILE *file_ptr_copy = fopen(inFileString, "r");
                bool defined = global_defined(relevant_label, file_ptr_copy);
                if(!defined) {
                    initialize_symbol(&s, relevant_label, 'U', 0);
                    push_back_symbol(&s, &all_symbols);
                }

            }

            // Put in relocation table if applicable
            if(strcmp(opcode, "beq") != 0 && relevant_label[0] != '\0') {
                int offset;
                if (data_yet) {
                    offset = data_size - 1;
                }
                else {
                    offset = text_size - 1;
                }
                struct Relocation r;
                initialize_relocation(&r, offset, opcode, relevant_label);
                push_back_relocation(&r, &all_relocation);
                
            }
        }
        if(strcmp(opcode, "beq") == 0) {
            if(arg2[0] != '\0') {
                if(isupper(arg2[0])) {
                    FILE *file_ptr_copy = fopen(inFileString, "r");
                    if(!global_defined(arg2, file_ptr_copy)){
                        printf("Error: beq using an undefined global symbolic address\n");
                        exit(1);
                    }
                }
                
            }
        }
        //If there is a label
        if(label[0] != '\0') {
            //If global
            if(isupper(label[0])) {
                if(!in_symbols(&all_symbols, label)) {
                    struct Symbol s;
                    char letter;
                    if(data_yet) {
                        letter = 'D';
                    }
                    else {
                        letter = 'T';
                    }
                    int offset;
                    if (data_yet) {
                        offset = data_size - 1;
                    }
                    else {
                        offset = text_size - 1;
                    }
                    initialize_symbol(&s, label, letter, offset);
                    push_back_symbol(&s, &all_symbols);
                }
            }
            
            if(check_duplicate_label(label, all_labels, idx) == 1) {
                printf("Error: Duplicate label\n");
                exit(1);
            }
            struct Label_holder label_holder1;
            strcpy(label_holder1.label, label);
            label_holder1.address = line_no;
            all_labels[idx] = label_holder1;
            ++idx;
        }
        ++line_no;
    }
    fprintf(outFilePtr, "%d %d %d %d\n", text_size, data_size, all_symbols.size, all_relocation.size);
    
    /* this is how to rewind the file ptr so that you start reading from the
        beginning of the file */
    rewind(inFilePtr);
    int pc = 0;
    while(readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)) {
        if(!strcmp(opcode, "lw") || !strcmp(opcode, "sw") || !strcmp(opcode, "beq")) {
            int output = itype(opcode, arg0, arg1, arg2, all_labels, idx, pc);
            print_info(outFilePtr, pc, output);
        }
        else if(!strcmp(opcode, "add") || !strcmp(opcode, "nor")) {
            int output = rtype(opcode, arg0, arg1, arg2, all_labels, idx, pc);
            print_info(outFilePtr, pc, output);
        }
        else if(!strcmp(opcode, "jalr")) {
            int output = jtype(opcode, arg0, arg1, all_labels, idx, pc);
            print_info(outFilePtr, pc, output);
        }
        else if(!strcmp(opcode, "halt") || !strcmp(opcode, "noop")) {
            int32_t final = 0;
            int opcode_int = opcode_binary(opcode);
            opcode_int = opcode_int << 22;
            final = final | opcode_int;
            print_info(outFilePtr, pc, final);
        }
        else if(!strcmp(opcode, ".fill")) {
            int output = fill(arg0, all_labels, idx);
            print_info(outFilePtr, pc, output);
            
        }
        else {
            printf("Error: Unrecognized opcodes\n");
            exit(1);
        }
        
        ++pc;
    }
    print_symbols(outFilePtr, &all_symbols);
    print_relocation(outFilePtr, &all_relocation);

    return(0);
}

/*
 * Read and parse a line of the assembly-language file.  Fields are returned
 * in label, opcode, arg0, arg1, arg2 (these strings must have memory already
 * allocated to them).
 *
 * Return values:
 *     0 if reached end of file
 *     1 if successfully read
 *
 * exit(1) if line is too long.
 */
int
readAndParse(FILE *inFilePtr, char *label, char *opcode, char *arg0,
        char *arg1, char *arg2)
{
    char line[MAXLINELENGTH];

    /* delete prior values */
    label[0] = opcode[0] = arg0[0] = arg1[0] = arg2[0] = '\0';

    /* read the line from the assembly-language file */
    if (fgets(line, MAXLINELENGTH, inFilePtr) == NULL) {
        /* reached end of file */
        return(0);
    }

    /* check for line too long (by looking for a \n) */
    if (strchr(line, '\n') == NULL) {
        /* line too long */
        printf("error: line too long\n");
        exit(1);
    }

    /* is there a label? */
    char *ptr = line;
    if (sscanf(ptr, "%[^\t\n\r ]", label)) {
        /* successfully read label; advance pointer over the label */
        ptr += strlen(label);
    }

    /*
     * Parse the rest of the line.  Would be nice to have real regular
     * expressions, but scanf will suffice.
     */
    sscanf(ptr, "%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]",
        opcode, arg0, arg1, arg2);
    return(1);
}

int
isNumber(char *string)
{
    /* return 1 if string is a number */
    int i;
    return( (sscanf(string, "%d", &i)) == 1);
}
bool global_defined(char* g, FILE *inFilePtr) {
    char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH],
            arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];
    while(readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)) {
        if(strcmp(label, g) == 0) {
            return true;
        }
    }
    return false;
}
void print_info(FILE *outFile, int pc, int binary) {
    fprintf(outFile, "%d\n", binary);
}
int fill(char* arg0, struct Label_holder *label_holder, int label_idx) {
    return get_label(arg0, label_holder, label_idx);
    
}
int jtype(char* opcode, char* regA, char* regB, struct Label_holder *label_holder, int label_idx, int pc) {
    int32_t final = 0;
    int opcode_int = opcode_binary(opcode);
    final = final | opcode_int;
    final = final << 22;
    
    int regA_int = get_label(regA, label_holder, label_idx);
    int regB_int = get_label(regB, label_holder, label_idx);
    
    regA_int = regA_int << 19;
    regB_int = regB_int << 16;
    final = final | regA_int;
    final = final | regB_int;
    return final;
    
}

int check_duplicate_label(char* label, struct Label_holder *label_holder, int label_idx) {
    for(int i = 0; i < label_idx; ++i) {
        if(!strcmp(label_holder[i].label, label)) {
            return 1;
        }
    }
    return 0;
}
int rtype(char* opcode, char* regA, char* regB, char* dest, struct Label_holder *label_holder, int label_idx, int pc) {
    int32_t final = 0;
    int opcode_int = opcode_binary(opcode);
    final = final | opcode_int;
    final = final << 22;
    
    int regA_int = get_label(regA, label_holder, label_idx);
    int regB_int = get_label(regB, label_holder, label_idx);
    int dest_int = get_label(dest, label_holder, label_idx);
    
    regA_int = regA_int << 19;
    regB_int = regB_int << 16;
    final = final | regA_int;
    final = final | regB_int;
    final = final | dest_int;
    
    return final;
    
    
}
int itype(char* opcode, char* regA, char* regB, char* offset, struct Label_holder *label_holder, int label_idx, int pc) {
    int32_t final = 0;
    int opcode_int = opcode_binary(opcode);
    final = final | opcode_int;
    final = final << 22;
    int regA_int = get_label(regA, label_holder, label_idx);
    int regB_int = get_label(regB, label_holder, label_idx);
    int offset_int = get_label(offset, label_holder, label_idx);
    
    if(offset_int > 32767 || offset_int < -32768) {
        printf("Error: Offset Does Not Fit in 16 bits\n");
        exit(1);
    }
    //If beq
    if(opcode_int == 0b100) {
        //If it's a label
        if(isNumber(offset) != 1) {
            offset_int = offset_int - pc - 1;
        }

    }
    int32_t offset2 = 0;
    offset2 = offset_int | offset2;
    int32_t mask = 0b00000000000000001111111111111111;
    offset2 = offset2 & mask;
    regA_int = regA_int << 19;
    regB_int = regB_int << 16;
    final = final | regA_int;
    final = final | regB_int;
    final = final | offset2;
    
    return final;
}
int get_label(char* reg, struct Label_holder *label_holder, int label_idx) {
    //If it's not a number
    if(isNumber(reg) != 1) {
        for(int i = 0; i < label_idx; ++i) {
            //If label is in the list, return address
            if(strcmp(label_holder[i].label, reg) == 0) {
                return label_holder[i].address;
            }
        }
        if(isupper(reg[0])) {
            return 0;
        }
        //Label is not in the list, error
        printf("Error: Undefined label\n");
        exit(1);
    }
    return atoi(reg);
}
int opcode_binary(char * opcode_in) {
    if(!strcmp(opcode_in, "lw")) {
        return 0b010;
    }
    else if(!strcmp(opcode_in, "sw")) {
        return 0b011;
    }
    else if(!strcmp(opcode_in, "beq")) {
        return 0b100;
    }
    else if(!strcmp(opcode_in, "add")) {
        return 0b000;
    }
    else if(!strcmp(opcode_in, "nor")) {
        return 0b001;
    }
    else if(!strcmp(opcode_in, "jalr")) {
        return 0b101;
    }
    else if(!strcmp(opcode_in, "halt")) {
        return 0b110;
    }
    else if(!strcmp(opcode_in, "noop")) {
        return 0b111;
    }
    
    printf("Unrecognized Opcode\n");
    exit(1);
}

