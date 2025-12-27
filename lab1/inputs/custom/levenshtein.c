#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define SEQ_LEN 1024

// function to find the minimum of three integers
uint32_t min(int32_t a, int32_t b, int32_t c) {
    if (a < b) return (a < c) ? a : c;
    return (b < c) ? b : c;
}


// function to generate pseudo random number with LCG
uint32_t lcg(uint32_t *state) {
    *state = (*state * 1664525 + 1013904223);
    return *state;
}

int main() {
    volatile uint32_t edit_distance = 0;
    uint32_t seed = 1337;
    uint32_t D[SEQ_LEN * SEQ_LEN];
    char seq1[SEQ_LEN];
    char seq2[SEQ_LEN];

    // initialize seq1 and seq2
    for (int32_t i = 0; i < SEQ_LEN; ++i) {
        seq1[i] = 'A';
        seq2[i] = 'A';
        if ((lcg(&seed) % 10) == 0) {
            seq2[i] = 'C';
        }
    }

    // fill dynamic programming matrix
    for (int32_t i = 0; i < SEQ_LEN; ++i) {
        for (int32_t j = 0; j < SEQ_LEN; ++j) {

            // internal cell (not upper or left edge)
            if (i != 0 && j != 0) {
                // check if we have a match
                if (seq1[i] == seq2[j]) {
                    D[i * SEQ_LEN + j] = D[(i-1) * SEQ_LEN + j-1];
                }

                if (seq1[i] != seq2[j]) {
                    D[i * SEQ_LEN + j] = min(D[(i-1) * SEQ_LEN + j], D[(i-1) * SEQ_LEN + j-1], D[i * SEQ_LEN + j-1]) + 1;
                }
            }

            // first col
            if (i != 0 && j == 0) {
                D[i * SEQ_LEN + j] = D[(i-1) * SEQ_LEN + j] + 1;
            }

            // first row
            if (i == 0 && j != 0) {
                D[i * SEQ_LEN + j] = D[i * SEQ_LEN + j-1] + 1;
            }

            // top left corner
            if (i == 0 && j == 0) {
                D[i * SEQ_LEN + j] = 0;
            }
        }
    }

    edit_distance = D[SEQ_LEN * SEQ_LEN - 1];

    return 0;
}