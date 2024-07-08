#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <fpe.h>
#include <fpe_locl.h>

/*
  usage:

  ./example 2DE79D232DF5585D68CE47882AE256D6 CBD09280979564 256

*/


int main(int argc, char *argv[])
{
    if (argc != 4) {
        printf("Usage: %s <key> <tweak> <radix>\n", argv[0]);
        return 0;
    }

    char* key = argv[1];
    char* tweak = argv[2];
    int radix = atoi(argv[3]);
    unsigned char plaintext[] = {0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20,
                                 0x61, 0x20, 0x74, 0x65, 0x73, 0x74};
    size_t txtlen = sizeof(plaintext) / sizeof(plaintext[0]); // 计算数组长度

    unsigned char *ciphertext = (unsigned char *)malloc(txtlen);
    unsigned char *resulttext = (unsigned char *)malloc(txtlen);

    FPE_KEY *ff1_key = FPE_ff1_create_key(key, tweak, radix);


    FPE_ff1_encrypt(plaintext, ciphertext, ff1_key, txtlen);
    printf("FF1 ciphertext: ");
    for (size_t i = 0; i < txtlen; ++i) {
        printf("%02x", ciphertext[i]);
    }
    printf("\n");


    // FF1解密
    FPE_ff1_decrypt(ciphertext, resulttext, ff1_key, txtlen);
    printf("FF1 decrypted:  ");
    for (size_t i = 0; i < txtlen; ++i) {
        printf("%02x", resulttext[i]);
    }
    printf("\n");


    FPE_ff1_delete_key(ff1_key);

    return 0;
}
