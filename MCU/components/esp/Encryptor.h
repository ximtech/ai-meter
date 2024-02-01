#pragma once

#include "../../src/AppConfig.h"

#include "psa_crypto_rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/error.h"
#include "mbedtls/aes.h"
#include "mbedtls/rsa.h"
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/platform.h>
#include "mbedtls/base64.h"

#define RSA_KEY_SIZE 2048
#define RSA_EXPONENT 65537
#define PUBLIC_KEY_PEM_SIZE 1000
#define PRIVATE_KEY_PEM_SIZE 2000

bool generateRsaKeyPair(File *publicKeyFile, File *privateKeyFile);
bool loadRsaEncryptionKeys(File *publicKeyFile, File *privateKeyFile);

size_t encryptTextWithPublicKey(const char *plainText, unsigned char *encrypted, uint32_t length);
size_t decryptTextWithPrivateKey(const unsigned char *encryptedMessage, size_t encryptedMessageLength, unsigned char *decrypted, uint32_t length);
BufferString *decryptBase64Text(char *encryptedText, BufferString *decrypted);

size_t decodeBase64(const char *inputBase64, size_t inputLength, unsigned char *outputBuffer, size_t outputBufferLength);

unsigned char *getPublicKeyString(size_t *keyLength);
