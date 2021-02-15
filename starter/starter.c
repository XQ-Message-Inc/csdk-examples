//  This project demonstrates basic usage of the XQ C SDK.
//  starter.c
//  starter
//
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <xq/xq.h>


int main(int argc, const char * argv[]) {
    
    if (argc < 2 ) {
        fprintf(stderr, "Usage: path_to_config_file email_address\n\n" );
        fprintf(stderr, "path_to_config_file: The path to the xq.ini configuration file containing your XQ API keys.\n");
        fprintf(stderr, "email_address: The email account to use for authorization. Your account confirmation links will be sent here.\n");
        exit(EXIT_FAILURE);
    }

    // 1. SDK Initialization
    struct xq_config cfg = xq_init( argv[1] );
    if (!xq_is_valid_config(&cfg) ) {
        // If something went wrong, call this to clean up
        // any memory that was possibly allocated.
        xq_destroy_config(&cfg);
        exit(EXIT_FAILURE);
    }
    
    // 2. Create Quantum Pool
    struct xq_quantum_pool pool = {0};
    struct xq_error_info err = {0};

    
    // 3. Authenticate a user.
    const char* email_address = argv[2];
    if  (!xq_svc_authorize( &cfg, email_address, &err )) {
        fprintf(stderr, "%li, %s", err.responseCode, err.content );
        xq_destroy_config(&cfg);
        exit(EXIT_FAILURE);
    }
    
    
    char pin[7] = {0};
    fprintf(stdout, "Enter PIN: ");
    fflush(stdout);
    fgets( pin, 7, stdin );
    int read = strcspn(pin, "\n");
    if ( read > 0 ) {
        
        pin[read] = 0;
        fprintf(stdout, "Attempting to authorize with PIN %s...\n", pin);
        if ( !xq_svc_code_validation( &cfg, pin , &err ) ) {
            fprintf(stderr, "%li, %s", err.responseCode, err.content );
            xq_destroy_config(&cfg);
            exit(EXIT_FAILURE);
        }
        
    }
    else {
        // If no PIN was provided, assume the link was clicked and attempt
        // an exchange.
        fprintf(stdout, "No PIN provided. Checking authorization state...\n");
        if  (!xq_svc_exchange( &cfg, &err )) {
            fprintf(stderr, "%li, %s", err.responseCode, err.content );
            xq_destroy_config(&cfg);
            exit(EXIT_FAILURE);
        }
    }
    
    fprintf(stdout, "Account authorized.\n");
    
    // Retrieving your access token
    const char* access_token = xq_get_access_token(&cfg);
    if ( !access_token ){
        xq_destroy_config(&cfg);
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Access Token: %s\n", access_token );
    
    xq_set_access_token(&cfg, access_token);
    {
        const char* access_token = xq_get_access_token(&cfg);
        if ( !access_token ){
            xq_destroy_config(&cfg);
            exit(EXIT_FAILURE);
        }
        fprintf(stdout, "Access Token: %s\n", access_token );
    }
    
    // Retrieve information about this user.
    struct xq_subscriber_info info = {0};
    if (!xq_svc_get_subscriber(&cfg, &info, &err)) {
        xq_destroy_config(&cfg);
        exit(EXIT_FAILURE);
    }

    
    // Initialize a sample quantum pool.
    if (!xq_init_pool(&cfg, 256, &pool, &err) ) {
        fprintf(stderr, "%li, %s", err.responseCode, err.content );
        xq_destroy_config(&cfg);
        exit(EXIT_FAILURE);
    }
    
    // 6. Encrypt a new message
    const char* message = "Hello World";
    const char* recipients = info.mailOrPhone;
    fprintf(stdout, "Encrypting message: %s...\n", message);

    struct xq_message_payload result = { 0,0 };

    if (!xq_encrypt_and_store_token(
        &cfg, // XQ Configuration object
        Algorithm_Autodetect, // The algorithm to use for encryption
        (uint8_t*)message,  // The message to encrypt.
        strlen(message), // The length of the message ( in bytes )
        64,  // The number entropy bytes to use.
        &pool, // Entropy pool to use ( 0 if none ).
        recipients, // The accounts that will be able to read this message.
        24, // The number of hours this message will be available
        0, // Prevent this message from being read more than once?
        &result,
        &err)) {
            fprintf(stderr, "%li, %s", err.responseCode, err.content );
            xq_destroy_pool(&pool);
            xq_destroy_config(&cfg);
            exit(EXIT_FAILURE);
        }

    // Success - The message has been successfully encrypted. The
    // encrypted message will be stored in "result.data".
    // Here, we will convert the result to base64 in order to display onscreen.
    struct xq_message_payload encoded = { 0, 0 };
    xq_base64_payload(&result, &encoded);
    // Display the encrypted message.
    fprintf(stdout, "Encrypted Message ( Base64 ): %s\n", encoded.data );
    // Display the XQ locator token.
    fprintf(stdout, "Token: %s\n", result.token_or_key);
    

    // The encrypted message should be exactly the same as
    // the one originally generated.
    struct xq_message_payload decrypted = { 0,0 };

    if (!xq_decrypt_with_token(
      &cfg,
      Algorithm_Autodetect, // The original algorithm (or autodetect)
      result.data,  // The encrypted payload
      result.length,  // The length of the encrypted payload
      result.token_or_key, // The XQ locator token
      &decrypted,
      &err)){
        fprintf(stderr, "%li, %s", err.responseCode, err.content );
        xq_destroy_pool(&pool);
        xq_destroy_config(&cfg);
        exit(EXIT_FAILURE);
    }

    // Success. The message has been successfully encrypted.
    fprintf(stdout, "Decrypted Message:%s\n", decrypted.data );
    fprintf(stdout, "Decrypted Length:%i\n", decrypted.length );
    xq_destroy_payload(&decrypted);
    
    
    // Revoke the entire message.
    if (!xq_svc_remove_key(&cfg, result.token_or_key, &err)) {
        fprintf(stderr, "%li, %s", err.responseCode, err.content );
        xq_destroy_pool(&pool);
        xq_destroy_config(&cfg);
        exit(EXIT_FAILURE);
    }
    
    // Cleanup
    xq_destroy_payload(&encoded);
    xq_destroy_payload(&result);
    
    
    // Success - Configuration should be safe to use from this point.
    xq_destroy_pool(&pool);
    xq_destroy_config(&cfg);
    
    return 0;
    
}
