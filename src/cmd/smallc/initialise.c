#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "data.h"

/*
 * SYMBOL_NAME 32
 * TYPE 1
 * LENGTH 2
 * initialisation
 */

/**
 * a constructor :-)
 */
void create_initials() {
    int i;
    for (i=0; i<INITIALS_SIZE; i++) {
        initials_table[i] = 0;
    }
}

/**
 * add new symbol to table
 * @param symbol_name
 */
void add_symbol(char *symbol_name, char type) {
    strcpy(initials_table_ptr, symbol_name);
    initials_table_ptr[INIT_TYPE] = type;
}

/**
 * find symbol in table
 * @param symbol_name
 * @return
 */
int find_symbol(char *symbol_name) {
    int result = 0;
    int index = 0;
    while (initials_table[index] != 0) {
        if (astreq (symbol_name, &initials_table[index], NAMEMAX) != 0) {
            result = 1;
            break;
        } else {
            // move to next symbol
            int length = ((unsigned char)initials_table[index + INIT_LENGTH] << 8) + (unsigned char)initials_table[index + INIT_LENGTH+1];
            index += NAMESIZE + 1 + 2 + (2 * (length));
        }
    }
    initials_table_ptr = &initials_table[index];
    return result;
}

/**
 * add data to table for given symbol
 * @param symbol_name
 * @param type
 * @param value
 */
void add_data(char *symbol_name, int type, int value) {
    int length, index;
    if (find_symbol(symbol_name) == 0) {
        add_symbol(symbol_name, type);
    }
    if (initials_table_ptr[INIT_TYPE] != type) {
        error("initialiser type mismatch");
    }
    length = ((unsigned char)initials_table_ptr[INIT_LENGTH] << 8) + (unsigned char)initials_table_ptr[INIT_LENGTH+1];
    index  = NAMESIZE + 1 + 2 + (2 * length);
    initials_table_ptr[index] = (0xff00 & value) >> 8;
    initials_table_ptr[index + 1] = 0xff & value;
    length++;
    initials_table_ptr[INIT_LENGTH] = (0xff00 & length) >> 8;
    initials_table_ptr[INIT_LENGTH+1] = 0xff & length;
}

/**
 * get number of data items for given symbol
 * @param symbol_name
 * @return
 */
int get_size(char *symbol_name) {
    int result = 0;
    if (find_symbol(symbol_name) != 0) {
        result = ((unsigned char)initials_table_ptr[INIT_LENGTH] << 8) + (unsigned char)initials_table_ptr[INIT_LENGTH+1];
    }
    return result;
}

/**
 * get item at position
 * @param symbol_name
 * @param position
 * @return
 */
int get_item_at(char *symbol_name, int position) {
    int result = 0;
    if (find_symbol(symbol_name) != 0) {
        int index  = NAMESIZE + 1 + 2 + (2 * position);
        result = (initials_table_ptr[index] << 8) + (unsigned char)initials_table_ptr[index+1];
    }
    return result;
}
