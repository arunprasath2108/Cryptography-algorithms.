#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/x509.h>
#include <openssl/kdf.h>
#include <iostream>
#include "ec_algorithm.hpp"
#include "utils.hpp" 
using namespace std;
       
// Function to convert the peer's generated key pair to a public key in DER format
void ECAlgorithm::ConvertToDER(EVP_PKEY* peer_key, unsigned char** peer_pub_der, int* peer_pub_der_len) {
    *peer_pub_der_len = i2d_PUBKEY(peer_key, peer_pub_der);
}

// Function to convert the received peer's public key from DER format
EVP_PKEY* ECAlgorithm::ConvertFromDER(const unsigned char* peer_pub_der, int peer_pub_der_len) {
    EVP_PKEY* peer_pub_key = d2i_PUBKEY(NULL, &peer_pub_der, peer_pub_der_len);
    return peer_pub_key;
}

// Function to derive a shared secret on the host using the host's key and the peer's public key
void DeriveSecret(EVP_PKEY* host_key, EVP_PKEY* peer_pub_key, unsigned char* secret_key) {
    
    unsigned int pad = 1;
    OSSL_PARAM params[2];
    size_t secret_len;
    EVP_PKEY_CTX* dctx = EVP_PKEY_CTX_new_from_pkey(NULL, host_key, NULL);

    EVP_PKEY_derive_init(dctx);

    // Optionally set the padding
    params[0] = OSSL_PARAM_construct_uint("X942KDF-ASN1", &pad);
    params[1] = OSSL_PARAM_construct_end();
    EVP_PKEY_CTX_set_params(dctx, params);

    EVP_PKEY_derive_set_peer(dctx, peer_pub_key);
    EVP_PKEY_derive(dctx, secret_key, &secret_len);
    EVP_PKEY_CTX_free(dctx);
} 

// Function to generate ECC key 
EVP_PKEY* ECAlgorithm::GenerateECKey()
{

    EVP_PKEY* pkey = NULL;
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);

    if (ctx == NULL)
    {
        std::cout << "error in ctx.\n";
        return NULL;
    }

    if (EVP_PKEY_keygen_init(ctx) <= 0)
    {
        std::cout << "error in init of keygen.\n";
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }

    if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_X9_62_prime256v1) <= 0)
    {
        std::cout << "error in setting EC params.\n";
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }

    if (EVP_PKEY_keygen(ctx, &pkey) <= 0)
    {
        std::cout << "error in generating key.\n";
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }

    EVP_PKEY_CTX_free(ctx);
    return pkey;
}

void ECAlgorithm::RunECAlgorithm() {

    //user A key-pair 
    EVP_PKEY* userA_keyPair = GenerateECKey();
    if (userA_keyPair == NULL) {   
        std::cout << "failed to generate key pair.\n";
    }

    //user B key-pair 
    EVP_PKEY* userB_keyPair = GenerateECKey();
    if (userB_keyPair == NULL) {   
        std::cout << "failed to generate key pair.\n";
    } 

    // Convert A's public-key pair to DER format
    unsigned char* userA_der_format = NULL;
    int userA_der_len;
    ConvertToDER(userA_keyPair, &userA_der_format, &userA_der_len);

    //convert B's key pair to DER(Distinguished Encoding Rules) format
    unsigned char* userB_der_format = NULL;
    int userB_der_len;
    ConvertToDER(userB_keyPair, &userB_der_format, &userB_der_len);

    // Convert received B's public key from DER format
    const unsigned char* der1 = userB_der_format;
    EVP_PKEY* userB_pub_key = ConvertFromDER(der1, userB_der_len);

    // Convert received A's public key from DER format
    const unsigned char* der2 = userA_der_format;
    EVP_PKEY* userA_pub_key = ConvertFromDER(der2, userA_der_len);

    // Derive a shared secret using the A's key and the B's public key
    int size = 30;
    unsigned char secret_A[size];
    DeriveSecret(userA_keyPair, userB_pub_key, secret_A);
    cout << "\nSecret derived by A : \n";
    PrintCipherText(secret_A, sizeof(secret_A));

    // Derive a shared secret using the A's key and the B's public key
    unsigned char secret_B[size];
    DeriveSecret(userB_keyPair, userA_pub_key, secret_B);
    cout << "\nSecret derived by B : \n";
    PrintCipherText(secret_B, sizeof(secret_B));

    EVP_PKEY_free(userA_keyPair);
    EVP_PKEY_free(userB_keyPair);
    EVP_PKEY_free(userA_pub_key);
    EVP_PKEY_free(userB_pub_key);
}