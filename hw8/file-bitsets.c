/*
** file-bitsets.c - Converts given char* strings or file contents to uint64
** and processes the resulting sets in a variety of ways.
**
** Author: Brandon Calabrese (bmc1340)
*/

#include <stdio.h>    // size_t type
#include <string.h>   // C strings
#include <stdint.h>   // uint64_t type
#include <stdlib.h>   // calloc

/*
** Declare constants to facilitate program functions
*/
const size_t SETSIZE = sizeof( uint64_t) << 3 ;
const size_t BUFSIZE = 256;
const char characters[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.0123456789,abcdefghijklmnopqrstuvwxyz";

uint64_t set_encode(char * st);

/*
** getBit: Get the position of a character in the "characters" array
** 
** parameter:
**  c the character to be tested for
**  
** returns:
** int representing where in the array the character appears (or -1)
*/
int getBit(char c) {
    for (unsigned int counter = 0; counter < SETSIZE; counter++) {
        if (c == characters[counter]) {
            return SETSIZE - 1 - counter;
        }
    }
    
    //if character not in accepted list return -1
    return -1;
}

/*
** file_set_encode: Encodes a file's contents into a set
** 
** parameter:
**  fp  the file which will have it's contents be processed into a set
**  
** returns:
** uint64_t representing the set
*/
uint64_t file_set_encode(FILE * fp) {
    
    char *buffer = (char*) calloc(BUFSIZE, sizeof(char) * BUFSIZE);
    fread(buffer, 1, BUFSIZE, fp);
    uint64_t result = set_encode(buffer);
    free(buffer);
    
    return result;
}

/*
** set_encode: Encodes a char * into a set
** 
** parameter:
**  st  the char* to be processed into a set
**  
** returns:
** uint64_t representing the set
*/
uint64_t set_encode(char * st) {
    
    uint64_t set = 0;
    
    char c = *st;
    int counter = 0;
    while (c != '\0') {
        int bitPlace = getBit(c);
        if (bitPlace != -1) {
            int bit = (set >> bitPlace) & 1U;
            if (bit == 0) {
                set ^= 1UL << bitPlace;
            }
        }

        counter++;
        c = *(st + counter);
    }
    
    return set;
}

/*
** set_intersect: Get the intersection of 2 sets
** 
** parameters:
**  set1  the first set to be checked
**  set2  the second set to be checked
**  
** returns:
** uint64_t representing the set of the intersection of 2 sets
*/
uint64_t set_intersect(uint64_t set1, uint64_t set2) {
    
    uint64_t set = 0;
    
    for (int counter = SETSIZE - 1; counter >= 0; counter--) {
        int bit1 = (set1 >> counter) & 1U;
        int bit2 = (set2 >> counter) & 1U;
        if (bit1 == 1 && bit2 == 1) {
            set ^= 1UL << counter;
        }
    }
    
    return set;
}

/*
** set_union: Get the union of 2 sets
** 
** parameters:
**  set1  the first set to be checked
**  set2  the second set to be checked
**  
** returns:
** uint64_t representing the set of the union of 2 sets
*/
uint64_t set_union(uint64_t set1, uint64_t set2) {
    
    uint64_t set = 0;
    
    for (int counter = SETSIZE - 1; counter >= 0; counter--) {
        int bit1 = (set1 >> counter) & 1U;
        int bit2 = (set2 >> counter) & 1U;
        if (bit1 == 1 || bit2 == 1) {
            set ^= 1UL << counter;
        }
    }
    
    return set;
}

/*
** set_complement: Get the complement of a set
** 
** parameters:
**  set  the set to be checked
**  
** returns:
** uint64_t representing the set of the complement of the set
*/
uint64_t set_complement(uint64_t set1) {
    
    uint64_t set = 0;
    
    for (int counter = SETSIZE - 1; counter >= 0; counter--) {
        int bit = (set1 >> counter) & 1U;
        if (bit == 0) {
            set ^= 1UL << counter;
        }
    }
    
    return set;
}

/*
** set_difference: Get the difference of 2 sets
** 
** parameters:
**  set1  the first set to be checked
**  set2  the second set to be checked
**  
** returns:
** uint64_t representing the set of the difference of 2 sets
*/
uint64_t set_difference(uint64_t set1, uint64_t set2) {
    
    uint64_t set = set1;
    
    for (int counter = SETSIZE - 1; counter >= 0; counter--) {
        int bit1 = (set1 >> counter) & 1U;
        int bit2 = (set2 >> counter) & 1U;
        if (bit1 == 1 && bit2 == 1) {
            set ^= 1UL << counter;
        }
    }
    
    return set;
}

/*
** set_symdifference: Get the symmetric difference of 2 sets
** 
** parameters:
**  set1  the first set to be checked
**  set2  the second set to be checked
**  
** returns:
** uint64_t representing the set of the symmetric difference of 2 sets
*/
uint64_t set_symdifference(uint64_t set1, uint64_t set2) {
    
    uint64_t set = 0;
    
    for (int counter = SETSIZE - 1; counter >= 0; counter--) {
        int bit1 = (set1 >> counter) & 1U;
        int bit2 = (set2 >> counter) & 1U;
        if (bit1 == 1 && bit2 == 0) {
            set ^= 1UL << counter;
        }
        if (bit1 == 0 && bit2 == 1) {
            set ^= 1UL << counter;
        }
    }
    
    return set;
}

/*
** set_cardinality: Get the cardinality (size) of a set
** 
** parameter:
**  set  the set to be checked
**  
** returns:
** size_t representing the cardinality of the set
*/
size_t set_cardinality(uint64_t set) {
    
    size_t count = 0;
    
    for (int counter = SETSIZE - 1; counter >= 0; counter--) {
        int bit = (set >> counter) & 1U;
        if (bit == 1) {
            count++;
        }
    }
    
    return count;
}

/*
** set_decode: Decode set into array of characters (synamically allocated)
** 
** parameter:
**  set  the set to be decoded
**  
** returns:
** char * representing decoded set
*/
char *set_decode(uint64_t set) {
    
    char *temp = (char*)calloc(SETSIZE, sizeof(char) * SETSIZE);
    
    int index = 0;
    for (int counter = SETSIZE - 1; counter >= 0; counter--) {
        int bit = (set >> counter) & 1U;
        if (bit == 1) {
            temp[index] = characters[SETSIZE - 1 - counter];
            index++;
        }
    }
    
    return temp;
}

/*
** main: Main function for program
** takes 2 command line arguments (character strings)
** creates a set for each input & apply the set operations
** reports the results of each operation
** 
** parameters:
**  argc  number of arguments
**  argv  collection of arguments
**  
** returns:
** 0 OR 1
*/
int main (int argc, char *argv[]) {
    
    //check for proper format / number of args
    if (argc != 3) {
        fprintf(stderr, "Usage: file-bitsets string1 string2\n");
        return 1;
    }
    
    //try opening file(s)
    //if no file found, try encoding literal word
    uint64_t set1 = 0;
    FILE * fp1 = NULL;
    fp1 = fopen(argv[1], "rb");
    if (fp1) {
        set1 = file_set_encode(fp1);
        fclose(fp1);
        printf("string1:\t%s\tEncoding the file:\t%s\n", argv[1], argv[1]);
    } else {
        set1 = set_encode(argv[1]);
        printf("string1:\t%s\tEncoding the string:\t%s\n", argv[1], argv[1]);
    }
    
    uint64_t set2 = 0;
    FILE * fp2 = NULL;
    fp2 = fopen(argv[2], "rb");
    if (fp2) {
        set2 = file_set_encode(fp2);
        fclose(fp2);
        printf("string2:\t%s\tEncoding the file:\t%s\n\n", argv[2], argv[2]);
    } else {
        set2 = set_encode(argv[2]);
        printf("string2:\t%s\tEncoding the string:\t%s\n\n", argv[2], argv[2]);
    }
    
    //display output    
    printf("set1:\t%#018lx\n", set1);
    printf("set2:\t%#018lx\n\n", set2);
    
    printf("set_intersect:\t%#018lx\n", set_intersect(set1, set2));
    printf("set_union:\t%#018lx\n\n", set_union(set1, set2));
    
    printf("set1 set_complement:\t%#018lx\n", set_complement(set1));
    printf("set2 set_complement:\t%#018lx\n\n", set_complement(set2));
    
    printf("set_difference:\t\t%#018lx\n", set_difference(set1, set2));
    printf("set_symdifference:\t%#018lx\n\n", set_symdifference(set1, set2));
    
    printf("set1 set_cardinality:\t%zu\n", set_cardinality(set1));
    printf("set2 set_cardinality:\t%zu\n\n", set_cardinality(set2));
    
    char *set1Temp = set_decode(set1);
    char *set2Temp = set_decode(set2);
    printf("members of set1:\t'%s'\n", set1Temp);
    printf("members of set2:\t'%s'\n", set2Temp);
    
    free(set1Temp);
    free(set2Temp);
    
    return 0;
}