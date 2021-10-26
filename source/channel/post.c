#include "cacert_pem.h"
#include <curl/curl.h>
#include <gccore.h>
#include <assert.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509.h>

// Override CA certificate with those bundled from cacert.pem.
static CURLcode ssl_ctx_callback(CURL *curl, void *ssl_ctx, void *userptr) {
    mbedtls_ssl_config *config = (mbedtls_ssl_config *)ssl_ctx;
    mbedtls_ssl_conf_ca_chain(config, (mbedtls_x509_crt *)userptr, NULL);

    return CURLE_OK;
}

s32 post_request(char *url, char *address, char *mac_address) {
    CURL *curl;
    CURLcode res;
    char arguments[5000];
    sprintf(arguments, "%s&%s", address, mac_address);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    struct curl_slist *headers=NULL;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Set Personal Data/1.0 (Nintendo Wii)");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, arguments);

    // Set CA certificate
    static mbedtls_x509_crt cacert;
    mbedtls_x509_crt_init(&cacert);
    s32 ret = mbedtls_x509_crt_parse(
        &cacert, (const unsigned char *)&cacert_pem, cacert_pem_size);
    if (ret < 0) {
        return ret;
    }

    curl_easy_setopt(curl, CURLOPT_SSL_CTX_DATA, &cacert);
    curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, ssl_ctx_callback);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        return (s32)res;
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    curl_global_cleanup();

    return 0;
}
