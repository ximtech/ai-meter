#include "Encryptor.h"

typedef struct RsaEncryptor {
    mbedtls_pk_context publicKeyContext;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctrDrbgContext;
    mbedtls_mpi N;
    mbedtls_mpi P;
    mbedtls_mpi Q;
    mbedtls_mpi D;
    mbedtls_mpi E;
    mbedtls_mpi DP;
    mbedtls_mpi DQ;
    mbedtls_mpi QP;
} RsaEncryptor;

static const char *TAG = "ENCRYPT";
static const char *SEED = "a47058a0-5429-45e9-941c-170dce6282c2";

static mbedtls_pk_context privateKey;
static mbedtls_pk_context publicKey;

static void deleteEncryptor(RsaEncryptor *encryptor);


bool generateRsaKeyPair(File *publicKeyFile, File *privateKeyFile) {
    LOG_INFO(TAG, "New encryption key creation started");
    RsaEncryptor encryptor = {0};

    mbedtls_ctr_drbg_init(&encryptor.ctrDrbgContext);
    mbedtls_pk_init(&encryptor.publicKeyContext);
    mbedtls_mpi_init(&encryptor.N);
    mbedtls_mpi_init(&encryptor.P);
    mbedtls_mpi_init(&encryptor.Q);
    mbedtls_mpi_init(&encryptor.D);
    mbedtls_mpi_init(&encryptor.E);
    mbedtls_mpi_init(&encryptor.DP);
    mbedtls_mpi_init(&encryptor.DQ);
    mbedtls_mpi_init(&encryptor.QP);

    LOG_INFO(TAG, "Seeding the random number generator...");
    mbedtls_entropy_init(&encryptor.entropy);
    int status = mbedtls_ctr_drbg_seed(&encryptor.ctrDrbgContext, mbedtls_entropy_func, &encryptor.entropy, (const unsigned char *) SEED, strlen(SEED));
    if (status != 0) {
        LOG_INFO(TAG, "Failed! mbedtls_ctr_drbg_seed returned [%d]", status);
        deleteEncryptor(&encryptor);
        return false;
    }

    status = mbedtls_pk_setup(&encryptor.publicKeyContext, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    if (status != 0) {
        LOG_INFO(TAG, "pk_setup failed: %i\n", status);
    }

    LOG_INFO(TAG, "Generating the RSA key [ %d-bit ]...", RSA_KEY_SIZE);
    status = mbedtls_rsa_gen_key(mbedtls_pk_rsa(encryptor.publicKeyContext), mbedtls_ctr_drbg_random, &encryptor.ctrDrbgContext, RSA_KEY_SIZE, RSA_EXPONENT);
    if (status != 0) {
        LOG_INFO(TAG, "Failed! mbedtls_rsa_gen_key returned [%d]", status);
        deleteEncryptor(&encryptor);
        return false;
    }

    LOG_INFO(TAG, "Checking public/private key validity...");
    if (mbedtls_rsa_check_pubkey(mbedtls_pk_rsa(encryptor.publicKeyContext)) != 0) {
        LOG_ERROR(TAG, "RSA context does not contain an rsa public key");
        deleteEncryptor(&encryptor);
        return false;
    }
    if (mbedtls_rsa_check_privkey(mbedtls_pk_rsa(encryptor.publicKeyContext)) != 0) {
        LOG_ERROR(TAG, "RSA context does not contain an rsa private key");
        deleteEncryptor(&encryptor);
        return false;
    }

    LOG_INFO(TAG, "Writing public key to string(PEM format)....");

    unsigned char pubKeyPem[PUBLIC_KEY_PEM_SIZE] = {0};
    if (mbedtls_pk_write_pubkey_pem(&encryptor.publicKeyContext, pubKeyPem, sizeof(pubKeyPem)) != 0) {
        LOG_ERROR(TAG, "Write public key to string failed");
        deleteEncryptor(&encryptor);
        return false;
    }
    LOG_DEBUG(TAG,"Public Key: [%s]", SUBSTRING_CSTR(64, (char*) pubKeyPem, 0, 63)->value);
    LOG_INFO(TAG, "Writing private key to string(PEM format)....");

    unsigned char privateKeyPem[PRIVATE_KEY_PEM_SIZE] = {0};
    status = mbedtls_pk_write_key_pem(&encryptor.publicKeyContext, privateKeyPem, sizeof(privateKeyPem));
    if (status != 0) {
        LOG_ERROR(TAG, "Write private key to string failed with code [%04x]", status);
    }

    LOG_DEBUG(TAG, "Private Key: [%s]", SUBSTRING_CSTR(64, (char *) privateKeyPem, 0, 63)->value);
    LOG_INFO(TAG, "Write keys to files....");
    uint32_t charsWritten = writeCharsToFile(publicKeyFile, (char *) pubKeyPem, PUBLIC_KEY_PEM_SIZE, false);
    if (charsWritten == 0) {
        LOG_ERROR(TAG, "Could not locate public key file at given path");
        deleteEncryptor(&encryptor);
        return false;
    }

    charsWritten = writeCharsToFile(privateKeyFile, (char *) privateKeyPem, PRIVATE_KEY_PEM_SIZE, false);
    if (charsWritten == 0) {
        LOG_ERROR(TAG, "Could not locate private key file at given path");
        deleteEncryptor(&encryptor);
        return false;
    }

    LOG_ERROR(TAG, "Success, process complete");
    deleteEncryptor(&encryptor);
    return true;
}

bool loadRsaEncryptionKeys(File *publicKeyFile, File *privateKeyFile) {
    mbedtls_pk_init(&privateKey);
    mbedtls_pk_init(&publicKey);

    LOG_INFO(TAG, "Encryption private key load start: [%s]", privateKeyFile->path);
    mbedtls_ctr_drbg_context ctx;
    mbedtls_ctr_drbg_init(&ctx);
    int status = mbedtls_pk_parse_keyfile(&privateKey, privateKeyFile->path, NULL, mbedtls_ctr_drbg_random, &ctx);
    if (status != 0) {
        char errorBuffer[128];
        mbedtls_strerror(status, errorBuffer, sizeof(errorBuffer));
        LOG_ERROR(TAG, "Failed to parse private key: [%s]", errorBuffer);
        mbedtls_pk_free(&privateKey);
        return false;
    }

    LOG_INFO(TAG, "Encryption public key load start: [%s]", publicKeyFile->path);
    status = mbedtls_pk_parse_public_keyfile(&publicKey, publicKeyFile->path);
    if (status != 0) {
        char errorBuffer[128];
        mbedtls_strerror(status, errorBuffer, sizeof(errorBuffer));
        LOG_ERROR(TAG, "Failed to parse public key: %s", errorBuffer);
        mbedtls_pk_free(&publicKey);
        return false;
    }

    LOG_INFO(TAG, "Encryption key load success");
    return true;
}

size_t encryptTextWithPublicKey(const char *plainText, unsigned char *encrypted, uint32_t length) { // RSAES-PKCS1-V1_5
    size_t plainTextLength = strlen(plainText);
    size_t outputLength = 0;
    memset(encrypted, 0, length);

    // Encryption using the parsed private key
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctrDrbgContext;
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctrDrbgContext);

    // Seed the random number generator
    int status = mbedtls_ctr_drbg_seed(&ctrDrbgContext, mbedtls_entropy_func, &entropy, (const unsigned char *) SEED, strlen(SEED));
    if (status != 0) {
        LOG_ERROR(TAG, "Error seeding random number generator");
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctrDrbgContext);
        return outputLength;
    }

    LOG_DEBUG(TAG, "Encrypting message with public key");
    status = mbedtls_pk_encrypt(&publicKey, (const unsigned char *) plainText, plainTextLength, encrypted, &outputLength, length, mbedtls_ctr_drbg_random, &ctrDrbgContext);

    if (status != 0) {
        char errorBuffer[128];
        mbedtls_strerror(status, errorBuffer, sizeof(errorBuffer));
        LOG_ERROR(TAG, "Error encrypting message: %s", errorBuffer);
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctrDrbgContext);
        return outputLength;
    }

    LOG_INFO(TAG, "Message encrypted!");
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctrDrbgContext);
    return outputLength;
}

size_t decryptTextWithPrivateKey(const unsigned char *encryptedMessage, size_t encryptedMessageLength, unsigned char *decrypted, uint32_t length) {
    memset(decrypted, 0, length);
    size_t outputLength = 0;

    // Decryption using the parsed private key
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctrDrbgContext;
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctrDrbgContext);

    // Seed the random number generator
    int status  = mbedtls_ctr_drbg_seed(&ctrDrbgContext, mbedtls_entropy_func, &entropy, (const unsigned char *) SEED, strlen(SEED));
    if (status != 0) {
        LOG_ERROR(TAG, "Error seeding random number generator");
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctrDrbgContext);
        return outputLength;
    }

    LOG_DEBUG(TAG, "Decrypting message with private key");
    status = mbedtls_pk_decrypt(&privateKey, encryptedMessage, encryptedMessageLength, decrypted, &outputLength, length, mbedtls_ctr_drbg_random, &ctrDrbgContext);
    if (status != 0) {
        char errorBuffer[128];
        mbedtls_strerror(status, errorBuffer, sizeof(errorBuffer));
        LOG_ERROR(TAG, "Error decrypting message: %s", errorBuffer);
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctrDrbgContext);
        return outputLength;
    }

    LOG_INFO(TAG, "Message decrypted!");
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctrDrbgContext);
    return outputLength;
}

BufferString *decryptBase64Text(char *encryptedText, BufferString *decrypted) {
    size_t encryptedTextLength = isStringNotBlank(encryptedText) ? strlen(encryptedText) : 0;
    if (encryptedTextLength == 0) return decrypted;

    unsigned char decoded[1024] = {0};
    size_t decodeLength = decodeBase64(encryptedText, encryptedTextLength, decoded, sizeof(decoded));
    size_t decryptedLength = decryptTextWithPrivateKey(decoded, decodeLength, (unsigned char *) decrypted->value, decrypted->capacity);
    decrypted->length = (uint32_t) decryptedLength;
    return decrypted;
}

unsigned char *getPublicKeyString(size_t *keyLength) {
    unsigned char *keyBuffer = NULL;
    File *publicRsaKeyFile = NEW_FILE(PUBLIC_KEY_PEM_FILE);
    int status = mbedtls_pk_load_file(publicRsaKeyFile->path, &keyBuffer, keyLength);
    if (status != 0) {
        mbedtls_free(keyBuffer);
        return NULL;
    }
    return keyBuffer;
}

size_t decodeBase64(const char *inputBase64, size_t inputLength, unsigned char *outputBuffer, size_t outputBufferLength) {
    size_t outputLength = 0;
    int decodeStatus = mbedtls_base64_decode(outputBuffer, outputBufferLength, &outputLength, (const unsigned char *) inputBase64, inputLength);
    if (decodeStatus != 0) {
        char errorBuffer[128];
        mbedtls_strerror(decodeStatus, errorBuffer, sizeof(errorBuffer));
        LOG_ERROR(TAG, "Error decoding base64: %s", errorBuffer);
        return 0;
    }
    return outputLength;
}

static void deleteEncryptor(RsaEncryptor *encryptor) {
    mbedtls_mpi_free(&encryptor->N);
    mbedtls_mpi_free(&encryptor->P);
    mbedtls_mpi_free(&encryptor->Q);
    mbedtls_mpi_free(&encryptor->D);
    mbedtls_mpi_free(&encryptor->E);
    mbedtls_mpi_free(&encryptor->DP);
    mbedtls_mpi_free(&encryptor->DQ);
    mbedtls_mpi_free(&encryptor->QP);
    mbedtls_pk_free(&encryptor->publicKeyContext);
    mbedtls_ctr_drbg_free(&encryptor->ctrDrbgContext);
    mbedtls_entropy_free(&encryptor->entropy);
}
