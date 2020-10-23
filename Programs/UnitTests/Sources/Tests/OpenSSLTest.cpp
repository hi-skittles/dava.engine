#ifdef __clang__ 
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

#include <random>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

#define CURL_STATICLIB
#include <curl/curl.h>

using namespace DAVA;

DAVA_TESTCLASS (OpenSSLTest)
{
    String password = "Hello Dava Engine!";
    Vector<double> data;
    Vector<unsigned char> encryptedData;

    OpenSSLTest()
    {
        FileSystem* fs = FileSystem::Instance();
        fs->DeleteDirectory("~doc:/OpensslTest/", true);
        fs->CreateDirectory("~doc:/OpensslTest/");

        std::random_device rd;
        std::mt19937 gen(rd());
        for (unsigned n = 0; n < 1024; ++n)
        {
            data.push_back(std::generate_canonical<double, 666>(gen));
        }
    }

    ~OpenSSLTest()
    {
        FileSystem* fs = FileSystem::Instance();
        fs->DeleteDirectory("~doc:/OpensslTest/", true);
    }

    String GetPrivateKeyFilePath()
    {
        return FilePath("~doc:/OpensslTest/PublicKey.key").GetAbsolutePathname();
    }

    String GetPublicKeyFilePath()
    {
        return FilePath("~doc:/OpensslTest/PrivateKey.key").GetAbsolutePathname();
    }

    DAVA_TEST (GenerateKeysTest)
    {
        FILE* privateKey = fopen(GetPrivateKeyFilePath().c_str(), "wb");
        FILE* publicKey = fopen(GetPublicKeyFilePath().c_str(), "wb");
        SCOPE_EXIT
        {
            if (privateKey != nullptr)
            {
                fclose(privateKey);
            }
            if (publicKey != nullptr)
            {
                fclose(publicKey);
            }
        };

        if (privateKey == nullptr || publicKey == nullptr)
        {
            TEST_VERIFY_WITH_MESSAGE(false, "Cannot create key files");
            return;
        }

        BIGNUM* bne = BN_new();
        BN_set_word(bne, RSA_F4);

        RSA* rsa = RSA_new();
        RSA_generate_key_ex(rsa, 1024, bne, nullptr);

        const EVP_CIPHER* cipher = EVP_get_cipherbyname("bf-ofb");
        SCOPE_EXIT
        {
            RSA_free(rsa);
            BN_free(bne);
        };

        char* pwdData = const_cast<char*>(password.c_str());
        int res = PEM_write_RSAPrivateKey(privateKey, rsa, cipher, nullptr, 0, nullptr, pwdData);
        TEST_VERIFY(res == 1);

        res = PEM_write_RSAPublicKey(publicKey, rsa);
        TEST_VERIFY(res == 1);
    }

    DAVA_TEST (EncryptTest)
    {
        FILE* publicKeyFile = fopen(GetPublicKeyFilePath().c_str(), "rb");
        RSA* publicKey = PEM_read_RSAPublicKey(publicKeyFile, nullptr, nullptr, nullptr);
        SCOPE_EXIT
        {
            if (publicKey != nullptr)
            {
                RSA_free(publicKey);
            }
            if (publicKeyFile != nullptr)
            {
                fclose(publicKeyFile);
            }
        };

        if (publicKeyFile == nullptr || publicKey == nullptr)
        {
            TEST_VERIFY_WITH_MESSAGE(false, "Cannot open or read public key file");
            return;
        }

        OpenSSL_add_all_algorithms();
        int allDataLen = static_cast<int>(data.size() * sizeof(decltype(data)::value_type));
        const unsigned char* ptr = reinterpret_cast<const unsigned char*>(data.data());
        Vector<unsigned char> tmp(RSA_size(publicKey));

        while (allDataLen != 0)
        {
            // must be less than RSA_size(rsa) - 11
            int dataLen = std::min(static_cast<int>(RSA_size(publicKey)) - 11, allDataLen);
            int res = RSA_public_encrypt(dataLen, ptr, tmp.data(), publicKey, RSA_PKCS1_PADDING);
            TEST_VERIFY(res == RSA_size(publicKey));

            ptr += dataLen;
            allDataLen -= dataLen;
            encryptedData.insert(encryptedData.end(), tmp.begin(), tmp.begin() + res);
        }
    }

    DAVA_TEST (DecryptTest)
    {
        OpenSSL_add_all_algorithms();
        FILE* privateKeyFile = fopen(GetPrivateKeyFilePath().c_str(), "rb");
        char* pwdData = const_cast<char*>(password.c_str());
        RSA* privateKey = PEM_read_RSAPrivateKey(privateKeyFile, nullptr, nullptr, pwdData);
        SCOPE_EXIT
        {
            if (privateKey != nullptr)
            {
                RSA_free(privateKey);
            }
            if (privateKeyFile != nullptr)
            {
                fclose(privateKeyFile);
            }
        };

        if (privateKeyFile == nullptr || privateKey == nullptr)
        {
            TEST_VERIFY_WITH_MESSAGE(false, "Cannot open or read private key file");
            return;
        }

        int keySize = RSA_size(privateKey);
        Vector<unsigned char> decryptedData;
        Vector<unsigned char> tmp(keySize);
        const unsigned char* ptr = reinterpret_cast<const unsigned char*>(encryptedData.data());
        int allDataLen = static_cast<int>(encryptedData.size());
        TEST_VERIFY(allDataLen % keySize == 0);

        while (allDataLen != 0)
        {
            int dataLen = RSA_private_decrypt(keySize, ptr, tmp.data(), privateKey, RSA_PKCS1_PADDING);
            TEST_VERIFY(dataLen >= 0 && dataLen <= keySize);

            allDataLen -= keySize;
            ptr += keySize;
            decryptedData.insert(decryptedData.end(), tmp.begin(), tmp.begin() + dataLen);
        }

        size_t originalDataLen = data.size() * sizeof(decltype(data)::value_type);
        TEST_VERIFY(originalDataLen == decryptedData.size());
        TEST_VERIFY(memcmp(data.data(), decryptedData.data(), originalDataLen) == 0);
    }

    static size_t WriteDummyCallback(void* contents, size_t size, size_t nmemb, void* userp)
    {
        size_t realsize = size * nmemb;
        return realsize;
    }

    DAVA_TEST (CurlWithSSL)
    {
        CURL* curl;
        CURLcode res;

        curl_global_init(CURL_GLOBAL_DEFAULT);

        curl = curl_easy_init();
        TEST_VERIFY(nullptr != curl)

        if (nullptr != curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, "https://www.google.com/");
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, 0);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteDummyCallback);

#if 1
            /*
            * If you want to connect to a site who isn't using a certificate that is
            * signed by one of the certs in the CA bundle you have, you can skip the
            * verification of the server's certificate. This makes the connection
            * A LOT LESS SECURE.
            *
            * If you have a CA cert for the server stored someplace else than in the
            * default bundle, then the CURLOPT_CAPATH option might come handy for
            * you.
            */
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

#if 1
            /*
            * If the site you're connecting to uses a different host name that what
            * they have mentioned in their server certificate's commonName (or
            * subjectAltName) fields, libcurl will refuse to connect. You can skip
            * this check, but this will make the connection less secure.
            */
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif

            /* Perform the request, res will get the return code */
            res = curl_easy_perform(curl);

            TEST_VERIFY(res == CURLE_OK);

            /* Check for errors */
            if (res == CURLE_OK)
            {
                Logger::Info("OK, curl was able to open https connection!");
            }
            else
            {
                Logger::Error(Format("curl_easy_perform() failed: %s\n", curl_easy_strerror(res)).c_str());
            }

            /* always cleanup */
            curl_easy_cleanup(curl);
        }

        curl_global_cleanup();
    }
};

#ifdef __clang__ 
#pragma clang diagnostic pop
#endif
