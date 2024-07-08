#include <stdint.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <openssl/aes.h>
#include <openssl/crypto.h>
#include <openssl/bn.h>
#include "fpe.h"
#include "fpe_locl.h"

void bytes2num(BIGNUM *Y, const unsigned char *X, unsigned long long radix, unsigned int len, BN_CTX *ctx) {
    BN_CTX_start(ctx);
    BIGNUM *r = BN_CTX_get(ctx), *x = BN_CTX_get(ctx);

    BN_set_word(Y, 0);
    BN_set_word(r, radix);
    for (int i = 0; i < len; ++i) {
        BN_set_word(x, X[i]);
        BN_mul(Y, Y, r, ctx);
        BN_add(Y, Y, x);
    }

    BN_CTX_end(ctx);
}

void num2bytes(const BIGNUM *X, unsigned char *Y, unsigned int radix, int len, BN_CTX *ctx) {
    BN_CTX_start(ctx);
    BIGNUM *dv = BN_CTX_get(ctx), *rem = BN_CTX_get(ctx), *r = BN_CTX_get(ctx), *XX = BN_CTX_get(ctx);

    BN_copy(XX, X);
    BN_set_word(r, radix);
    memset(Y, 0, len);

    for (int i = len - 1; i >= 0; --i) {
        BN_div(dv, rem, XX, r, ctx);
        Y[i] = BN_get_word(rem);
        BN_copy(XX, dv);
    }

    BN_CTX_end(ctx);
}

void FF1_encrypt(const unsigned char *plaintext, unsigned char *ciphertext, FPE_KEY *key, const unsigned char *tweak, size_t txtlen, size_t tweaklen) {
    BIGNUM *bnum = BN_new(), *y = BN_new(), *c = BN_new(), *anum = BN_new(), *qpow_u = BN_new(), *qpow_v = BN_new();
    BN_CTX *ctx = BN_CTX_new();

    union {
        long one;
        char little;
    } is_endian = { 1 };

    int u = floor2(txtlen, 1);
    int v = txtlen - u;

    memcpy(ciphertext, plaintext, txtlen);
    unsigned char *A = ciphertext;
    unsigned char *B = ciphertext + u;

    pow_uv(qpow_u, qpow_v, key->radix, u, v, ctx);

    unsigned int temp = (unsigned int)ceil(v * log2(key->radix));
    const int b = ceil2(temp, 3);
    const int d = 4 * ceil2(b, 2) + 4;

    int pad = ((-tweaklen - b - 1) % 16 + 16) % 16;
    int Qlen = tweaklen + pad + 1 + b;
    unsigned char P[16];
    unsigned char *Q = (unsigned char *)OPENSSL_malloc(Qlen), *Bytes = (unsigned char *)OPENSSL_malloc(b);

    P[0] = 0x1;
    P[1] = 0x2;
    P[2] = 0x1;
    P[7] = u % 256;
    if (is_endian.little) {
        temp = (key->radix << 8) | 10;
        P[3] = (temp >> 24) & 0xff;
        P[4] = (temp >> 16) & 0xff;
        P[5] = (temp >> 8) & 0xff;
        P[6] = temp & 0xff;
        P[8] = (txtlen >> 24) & 0xff;
        P[9] = (txtlen >> 16) & 0xff;
        P[10] = (txtlen >> 8) & 0xff;
        P[11] = txtlen & 0xff;
        P[12] = (tweaklen >> 24) & 0xff;
        P[13] = (tweaklen >> 16) & 0xff;
        P[14] = (tweaklen >> 8) & 0xff;
        P[15] = tweaklen & 0xff;
    } else {
        *( (unsigned int *)(P + 3) ) = (key->radix << 8) | 10;
        *( (unsigned int *)(P + 8) ) = txtlen;
        *( (unsigned int *)(P + 12) ) = tweaklen;
    }

    memcpy(Q, tweak, tweaklen);
    memset(Q + tweaklen, 0x00, pad);
    assert(tweaklen + pad - 1 <= Qlen);

    unsigned char R[16];
    int cnt = ceil2(d, 4) - 1;
    int Slen = 16 + cnt * 16;
    unsigned char *S = (unsigned char *)OPENSSL_malloc(Slen);
    for (int i = 0; i < FF1_ROUNDS; ++i) {
        int m = (i & 1) ? v : u;

        Q[tweaklen + pad] = i & 0xff;
        bytes2num(bnum, B, key->radix, txtlen - m, ctx);
        int BytesLen = BN_bn2bin(bnum, Bytes);
        memset(Q + Qlen - b, 0x00, b);

        int qtmp = Qlen - BytesLen;
        memcpy(Q + qtmp, Bytes, BytesLen);

        AES_encrypt(P, R, &key->aes_enc_ctx);
        int count = Qlen / 16;
        unsigned char Ri[16];
        unsigned char *Qi = Q;
        for (int cc = 0; cc < count; ++cc) {
            for (int j = 0; j < 16; ++j)    Ri[j] = Qi[j] ^ R[j];
            AES_encrypt(Ri, R, &key->aes_enc_ctx);
            Qi += 16;
        }

        unsigned char tmp[16], SS[16];
        memset(S, 0x00, Slen);
        assert(Slen >= 16);
        memcpy(S, R, 16);
        for (int j = 1; j <= cnt; ++j) {
            memset(tmp, 0x00, 16);

            if (is_endian.little) {
                tmp[15] = j & 0xff;
                tmp[14] = (j >> 8) & 0xff;
                tmp[13] = (j >> 16) & 0xff;
                tmp[12] = (j >> 24) & 0xff;
            } else *( (unsigned int *)tmp + 3 ) = j;

            for (int k = 0; k < 16; ++k)    tmp[k] ^= R[k];
            AES_encrypt(tmp, SS, &key->aes_enc_ctx);
            assert((S + 16 * j)[0] == 0x00);
            assert(16 + 16 * j <= Slen);
            memcpy(S + 16 * j, SS, 16);
        }

        BN_bin2bn(S, d, y);
        bytes2num(anum, A, key->radix, m, ctx);
        if (m == u)    BN_mod_add(c, anum, y, qpow_u, ctx);
        else    BN_mod_add(c, anum, y, qpow_v, ctx);

        unsigned char *tmp_ptr = A;
        A = B;
        B = tmp_ptr;
        num2bytes(c, B, key->radix, m, ctx);
    }

    BN_clear_free(anum);
    BN_clear_free(bnum);
    BN_clear_free(c);
    BN_clear_free(y);
    BN_clear_free(qpow_u);
    BN_clear_free(qpow_v);
    BN_CTX_free(ctx);
    OPENSSL_free(Bytes);
    OPENSSL_free(Q);
    OPENSSL_free(S);
}

void FF1_decrypt(const unsigned char *ciphertext, unsigned char *plaintext, FPE_KEY *key, const unsigned char *tweak, size_t txtlen, size_t tweaklen) {
    BIGNUM *bnum = BN_new(), *y = BN_new(), *c = BN_new(), *anum = BN_new(), *qpow_u = BN_new(), *qpow_v = BN_new();
    BN_CTX *ctx = BN_CTX_new();

    union {
        long one;
        char little;
    } is_endian = { 1 };

    int u = floor2(txtlen, 1);
    int v = txtlen - u;

    memcpy(plaintext, ciphertext, txtlen);
    unsigned char *A = plaintext;
    unsigned char *B = plaintext + u;
    pow_uv(qpow_u, qpow_v, key->radix, u, v, ctx);

    unsigned int temp = (unsigned int)ceil(v * log2(key->radix));
    const int b = ceil2(temp, 3);
    const int d = 4 * ceil2(b, 2) + 4;

    int pad = ((-tweaklen - b - 1) % 16 + 16) % 16;
    int Qlen = tweaklen + pad + 1 + b;
    unsigned char P[16];
    unsigned char *Q = (unsigned char *)OPENSSL_malloc(Qlen), *Bytes = (unsigned char *)OPENSSL_malloc(b);

    P[0] = 0x1;
    P[1] = 0x2;
    P[2] = 0x1;
    P[7] = u % 256;
    if (is_endian.little) {
        temp = (key->radix << 8) | 10;
        P[3] = (temp >> 24) & 0xff;
        P[4] = (temp >> 16) & 0xff;
        P[5] = (temp >> 8) & 0xff;
        P[6] = temp & 0xff;
        P[8] = (txtlen >> 24) & 0xff;
        P[9] = (txtlen >> 16) & 0xff;
        P[10] = (txtlen >> 8) & 0xff;
        P[11] = txtlen & 0xff;
        P[12] = (tweaklen >> 24) & 0xff;
        P[13] = (tweaklen >> 16) & 0xff;
        P[14] = (tweaklen >> 8) & 0xff;
        P[15] = tweaklen & 0xff;
    } else {
        *( (unsigned int *)(P + 3) ) = (key->radix << 8) | 10;
        *( (unsigned int *)(P + 8) ) = txtlen;
        *( (unsigned int *)(P + 12) ) = tweaklen;
    }

    memcpy(Q, tweak, tweaklen);
    memset(Q + tweaklen, 0x00, pad);
    assert(tweaklen + pad - 1 <= Qlen);

    unsigned char R[16];
    int cnt = ceil2(d, 4) - 1;
    int Slen = 16 + cnt * 16;
    unsigned char *S = (unsigned char *)OPENSSL_malloc(Slen);
    for (int i = FF1_ROUNDS - 1; i >= 0; --i) {
        int m = (i & 1) ? v : u;

        Q[tweaklen + pad] = i & 0xff;
        bytes2num(anum, A, key->radix, txtlen - m, ctx);
        memset(Q + Qlen - b, 0x00, b);
        int BytesLen = BN_bn2bin(anum, Bytes);
        int qtmp = Qlen - BytesLen;
        memcpy(Q + qtmp, Bytes, BytesLen);

        memset(R, 0x00, sizeof(R));
        AES_encrypt(P, R, &key->aes_enc_ctx);
        int count = Qlen / 16;
        unsigned char Ri[16];
        unsigned char *Qi = Q;
        for (int cc = 0; cc < count; ++cc) {
            for (int j = 0; j < 16; ++j)    Ri[j] = Qi[j] ^ R[j];
            AES_encrypt(Ri, R, &key->aes_enc_ctx);
            Qi += 16;
        }

        unsigned char tmp[16], SS[16];
        memset(S, 0x00, Slen);
        memcpy(S, R, 16);
        for (int j = 1; j <= cnt; ++j) {
            memset(tmp, 0x00, 16);

            if (is_endian.little) {
                tmp[15] = j & 0xff;
                tmp[14] = (j >> 8) & 0xff;
                tmp[13] = (j >> 16) & 0xff;
                tmp[12] = (j >> 24) & 0xff;
            } else *( (unsigned int *)tmp + 3 ) = j;

            for (int k = 0; k < 16; ++k)    tmp[k] ^= R[k];
            AES_encrypt(tmp, SS, &key->aes_enc_ctx);
            assert((S + 16 * j)[0] == 0x00);
            memcpy(S + 16 * j, SS, 16);
        }

        BN_bin2bn(S, d, y);
        bytes2num(bnum, B, key->radix, m, ctx);
        if (m == u)    BN_mod_sub(c, bnum, y, qpow_u, ctx);
        else    BN_mod_sub(c, bnum, y, qpow_v, ctx);

        unsigned char *tmp_ptr = A;
        A = B;
        B = tmp_ptr;
        num2bytes(c, A, key->radix, m, ctx);
    }

    BN_clear_free(anum);
    BN_clear_free(bnum);
    BN_clear_free(y);
    BN_clear_free(c);
    BN_clear_free(qpow_u);
    BN_clear_free(qpow_v);
    BN_CTX_free(ctx);
    OPENSSL_free(Bytes);
    OPENSSL_free(Q);
    OPENSSL_free(S);
}

int create_ff1_key(const unsigned char *userKey, const int bits, const unsigned char *tweak, const unsigned int tweaklen, const unsigned int radix, FPE_KEY *key)
{
    int ret;
    if (bits != 128 && bits != 192 && bits != 256) {
        ret = -1;
        return ret;
    }
    key->tweaklen = tweaklen;
    key->tweak = (unsigned char *)OPENSSL_malloc(tweaklen);
    memcpy(key->tweak, tweak, tweaklen);
    ret = AES_set_encrypt_key(userKey, bits, &key->aes_enc_ctx);
    key->radix = radix;
    return ret;
}

FPE_KEY* FPE_ff1_create_key(const char *key, const char *tweak, unsigned int radix)
{
    unsigned char k[100],
            t[100];
    int klen = strlen(key) / 2;
    int tlen = strlen(tweak) / 2;

    hex2chars(key, k);
    hex2chars(tweak, t);

    FPE_KEY *keystruct  = (FPE_KEY *)OPENSSL_malloc(sizeof(FPE_KEY));
    create_ff1_key(k,klen*8,t,tlen,radix,keystruct);
    return keystruct;
}

void FPE_ff1_delete_key(FPE_KEY *key)
{
    OPENSSL_clear_free(key->tweak,key->tweaklen/8);
    OPENSSL_clear_free(key,sizeof(key));
}


/*
 * 通过参数传递txtlen
 * sizeof 在指针上调用时只会返回指针的大小，而不是它所指向的数组的大小
 */
void FPE_ff1_encrypt(const unsigned char *plaintext, unsigned char *ciphertext, FPE_KEY *key,  size_t txtlen) {
//    int txtlen = sizeof(plaintext);

    printf("plaintext: ");
    for (int i = 0; i < txtlen; ++i)    printf("%02x", plaintext[i]);
    printf("\n\n");

    FF1_encrypt(plaintext, ciphertext, key, key->tweak, txtlen, key->tweaklen);
}


void FPE_ff1_decrypt(const unsigned char *ciphertext, unsigned char *plaintext, FPE_KEY *key, size_t txtlen) {
//    int txtlen = sizeof(ciphertext);

    FF1_decrypt(ciphertext, plaintext, key, key->tweak, txtlen, key->tweaklen);
}