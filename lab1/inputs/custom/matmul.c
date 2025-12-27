#include <stdint.h>

#define MAT_SIZE_ROOT 128
#define MAT_SIZE (MAT_SIZE_ROOT * MAT_SIZE_ROOT)

int main() {
  volatile int32_t mat[MAT_SIZE];
  int32_t mat1[MAT_SIZE];
  int32_t mat2[MAT_SIZE];

  // initialize matrices
  for (int32_t i = 0; i < MAT_SIZE; ++i) {
    mat[i] = 0;
    mat1[i] = i;
    mat2[i] = MAT_SIZE - i;
  }

  // perform matrix multiplication
  for (int32_t i = 0; i < MAT_SIZE_ROOT; ++i) {
    for (int32_t j = 0; j < MAT_SIZE_ROOT; ++j) {
      for (int32_t k = 0; k < MAT_SIZE_ROOT; ++k) {
        mat[i * MAT_SIZE_ROOT + j] += mat1[i * MAT_SIZE_ROOT + k] * mat2[k * MAT_SIZE_ROOT + j];
      }
    }
  }

  return 0;
}