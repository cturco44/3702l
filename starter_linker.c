/**
 * Project 2
 * LC-2K Linker
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAXSIZE 300
#define MAXLINELENGTH 1000
#define MAXFILES 6

typedef struct FileData FileData;
typedef struct SymbolTableEntry SymbolTableEntry;
typedef struct RelocationTableEntry RelocationTableEntry;
typedef struct CombinedFiles CombinedFiles;
typedef struct DefinedSymbol DefinedSymbol;

struct DefinedSymbol {
    char label[7];
    int address;
    int file;
};

struct SymbolTableEntry {
	char label[7];
	char location;
	int offset;
};

struct RelocationTableEntry {
	int offset;
	char inst[7];
	char label[7];
	int file;
};

struct FileData {
	int textSize;
	int dataSize;
	int symbolTableSize;
	int relocationTableSize;
	int textStartingLine; // in final executible
	int dataStartingLine; // in final executible
	int text[MAXSIZE];
	int data[MAXSIZE];
	SymbolTableEntry symbolTable[MAXSIZE];
	RelocationTableEntry relocTable[MAXSIZE];
};

struct CombinedFiles {
	int text[MAXSIZE];
	int data[MAXSIZE];
	SymbolTableEntry     symTable[MAXSIZE];
	RelocationTableEntry relocTable[MAXSIZE];
	int textSize;
	int dataSize;
	int symTableSize;
	int relocTableSize;
};
int get_total_size(int file, FileData arr[]);
void check_duplicate_label(DefinedSymbol allSymbols[], int size, char* label);
void check_undefined_label(DefinedSymbol allSymbols[], int size, char* label);
int get_new_address(FileData arr[], int file_idx, int idx, int total_text_size, bool is_data);
int extractBits(int num, int position, int num_bits);
DefinedSymbol find_symbol(DefinedSymbol allSymbols[], int size, char* label);
int main(int argc, char *argv[])
{
	char *inFileString, *outFileString;
	FILE *inFilePtr, *outFilePtr; 
	int i, j;

	if (argc <= 2) {
		printf("error: usage: %s <obj file> ... <output-exe-file>\n",
				argv[0]);
		exit(1);
	}

	outFileString = argv[argc - 1];

	outFilePtr = fopen(outFileString, "w");
	if (outFilePtr == NULL) {
		printf("error in opening %s\n", outFileString);
		exit(1);
	}

	FileData files[MAXFILES];
    int numFiles = 0;
	//Reads in all files and combines into master
	for (i = 0; i < argc - 2; i++) {
		inFileString = argv[i+1];

		inFilePtr = fopen(inFileString, "r");
		printf("opening %s\n", inFileString);

		if (inFilePtr == NULL) {
			printf("error in opening %s\n", inFileString);
			exit(1);
		}

		char line[MAXLINELENGTH];
		int sizeText, sizeData, sizeSymbol, sizeReloc;

		// parse first line
		fgets(line, MAXSIZE, inFilePtr);
		sscanf(line, "%d %d %d %d",
				&sizeText, &sizeData, &sizeSymbol, &sizeReloc);

		files[i].textSize = sizeText;
		files[i].dataSize = sizeData;
		files[i].symbolTableSize = sizeSymbol;
		files[i].relocationTableSize = sizeReloc;

		// read in text
		int instr;
		for (j = 0; j < sizeText; j++) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			instr = atoi(line);
			files[i].text[j] = instr;
		}

		// read in data
		int data;
		for (j = 0; j < sizeData; j++) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			data = atoi(line);
			files[i].data[j] = data;
		}

		// read in the symbol table
		char label[7];
		char type;
		int addr;
		for (j = 0; j < sizeSymbol; j++) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			sscanf(line, "%s %c %d",
					label, &type, &addr);
			files[i].symbolTable[j].offset = addr;
			strcpy(files[i].symbolTable[j].label, label);
			files[i].symbolTable[j].location = type;
		}

		// read in relocation table
		char opcode[7];
		for (j = 0; j < sizeReloc; j++) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			sscanf(line, "%d %s %s",
					&addr, opcode, label);
			files[i].relocTable[j].offset = addr;
			strcpy(files[i].relocTable[j].inst, opcode);
			strcpy(files[i].relocTable[j].label, label);
			files[i].relocTable[j].file	= i;
		}
		fclose(inFilePtr);
        numFiles = i + 1;
	} // end reading files
    int total_text_size = 0;
    for(int i = 0; i < numFiles; ++i) {
        total_text_size += files[i].textSize;
    }
    DefinedSymbol allSymbols[600];
    int all_symbols_size = 0;
    for (int file = 0; file < numFiles; ++file) {
        int numSymbols = files[file].symbolTableSize;
        for(int i = 0; i < numSymbols; ++i) {
            if(files[file].symbolTable[i].location == 'T') {
                DefinedSymbol x;
                x.address = get_total_size(file, files) + files[file].symbolTable[i].offset;
                strcpy(x.label, files[file].symbolTable[i].label);
                x.file = file;
                check_duplicate_label(allSymbols, all_symbols_size, x.label);
                allSymbols[all_symbols_size] = x;
                ++all_symbols_size;
            }
            else if (files[file].symbolTable[i].location == 'D') {
                DefinedSymbol x;
                x.address = get_total_size(file, files) + files[file].symbolTable[i].offset + files[file].textSize;
                strcpy(x.label, files[file].symbolTable[i].label);
                x.file = file;
                check_duplicate_label(allSymbols, all_symbols_size, x.label);
                allSymbols[all_symbols_size] = x;
                ++all_symbols_size;
            }
        }
    }
    
    //Check undefined labels
    for (int file = 0; file < numFiles; ++file) {
        int numSymbols = files[file].symbolTableSize;
        for(int i = 0; i < numSymbols; ++i) {
            if(files[file].symbolTable[i].location == 'U') {
                check_undefined_label(allSymbols, all_symbols_size, files[file].symbolTable[i].label);
            }
        }
    }
    
    for(int file = 0; file < numFiles; ++file) {
        int numRelocation = files[file].relocationTableSize;
        for(int i = 0; i < numRelocation; ++i) {
            //If lw or sw
            char first_letter = file[files].relocTable[i].label[0];
            if(strcmp(files[file].relocTable[i].inst, ".fill") != 0 && isupper(first_letter) == 0) {
                int idx = files[file].relocTable[i].offset;
                int offset = extractBits(files[file].text[idx], 0, 16);
                int data_idx = offset % files[file].textSize;
                int new_address = get_new_address(files, file, data_idx, total_text_size, true);
                files[file].text[idx] += new_address - offset;
            }
            //.fill
            else if(isupper(first_letter) == 0) {
                int data_idx = files[file].relocTable[i].offset;
                int text_idx = files[file].data[data_idx];
                int new_address = get_new_address(files, file, text_idx, total_text_size, false);
                files[file].data[data_idx] += new_address - text_idx;
                                         
            }
            //global lw or sw
            else if(strcmp(files[file].relocTable[i].inst, ".fill") != 0) {
                DefinedSymbol x = find_symbol(allSymbols, all_symbols_size, files[file].relocTable[i].label);
                //Label defined here
                if(file == x.file) {
                    int idx = files[file].relocTable[i].offset;
                    int offset = extractBits(files[file].text[idx], 0, 16);
                    int data_idx = offset % files[file].textSize;
                    int new_address = get_new_address(files, file, data_idx, total_text_size, true);
                    files[file].text[idx] += new_address - offset;
                }
                else {
                    int text_idx = files[file].relocTable[i].offset;
                    files[file].text[text_idx] += x.address;
                }
            }
            //.fill
            else {
                DefinedSymbol x = find_symbol(allSymbols, all_symbols_size, files[file].relocTable[i].label);
                //Label defined here
                if(file == x.file) {
                    int data_idx = files[file].relocTable[i].offset;
                    int text_idx = files[file].data[data_idx];
                    int new_address = get_new_address(files, file, text_idx, total_text_size, false);
                    files[file].data[data_idx] += new_address - text_idx;
                }
                else {
                    int data_idx = files[file].relocTable[i].offset;
                    files[file].data[data_idx] = x.address;
                }
            }
        }
    }
    
    
    for(int i = 0; i < numFiles; ++i) {
        for(int t = 0; t < files[i].textSize; ++t) {
            fprintf(outFilePtr, "%d\n", files[i].text[t]);
        }
    }
    for(int i = 0; i < numFiles; ++i) {
        for(int d = 0; d < files[i].dataSize; ++d) {
            fprintf(outFilePtr, "%d\n", files[i].data[d]);
        }
    }
    

	// *** INSERT YOUR CODE BELOW ***
	//    Begin the linking process
	//    Happy coding!!!

} // end main
DefinedSymbol find_symbol(DefinedSymbol allSymbols[], int size, char* label) {
    for(int i = 0; i < size; ++i) {
        if(strcmp(allSymbols[i].label, label) == 0) {
            return allSymbols[i];
        }
    }
    printf("Error symbol not found");
    exit(1);
}
int get_new_address(FileData arr[], int file_idx, int idx, int total_text_size, bool is_data) {
    if(is_data) {
        int prior_data_size = 0;
        for(int i = 0; i < file_idx; ++i) {
            prior_data_size += arr[i].dataSize;
        }
        return total_text_size + prior_data_size + idx;
    }
    int prior_text_size = 0;
    for(int i = 0; i < file_idx; ++i) {
        prior_text_size += arr[i].textSize;
    }
    return prior_text_size + idx;

}
int extractBits(int num, int position, int num_bits) {
    int mask = 1;
    mask = mask << num_bits;
    mask = mask - 1;
    
    num = num >> position;
    return num & mask;
}
void check_undefined_label(DefinedSymbol allSymbols[], int size, char* label) {
    for(int i = 0; i < size; ++i) {
        if(strcmp(allSymbols[i].label, label) == 0) {
            return;
        }
    }
    printf("Label undefined");
    exit(1);
}

void check_duplicate_label(DefinedSymbol allSymbols[], int size, char* label) {
    for(int i = 0; i < size; ++i) {
        if(strcmp(allSymbols[i].label, label) == 0) {
            printf("Duplicate Defined Label");
            exit(1);
        }
    }
}
int get_total_size(int file, FileData arr[]) {
    int total = 0;
    for (int i = 0; i < file; ++i) {
        total += arr[i].textSize;
        total += arr[i].dataSize;
    }
    return total;
}
