#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "../jsml.h"

// Struct to hold response data and its size
struct MemoryStruct {
    char* memory;
    size_t size;
};

// Callback function to handle the response data
size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct* mem = (struct MemoryStruct*)userp;

    // Allocate memory for the new data and append it to the existing data
    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = '\0';

    return realsize;
}

int main() {
    CURL* curl;
    CURLcode res;

    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Create a curl handle
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error initializing libcurl\n");
        return 1;
    }

    // Set the URL to GitHub API endpoint
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/users/gaurav-gosain");

    // Set the callback function to handle the response data
    struct MemoryStruct chunk;
    chunk.memory = malloc(1); // Initialize with an empty string
    chunk.size = 0;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);

    // Enable response encoding (gzip, deflate) decoding
    curl_easy_setopt(curl, CURLOPT_ENCODING, "");

    // Set the user agent (GitHub API requires a User-Agent header)
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    // Perform the HTTP request
    res = curl_easy_perform(curl);

    // Check for errors
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
    else {
        // Print the received JSON data
        printf("Response:\n%s\n", chunk.memory);

        // Parse JSON using your own parsing function (json_parse_utf8)
        const json* parsed_json = json_parse_utf8(chunk.memory);
        if (parsed_json) {
            // Print the parsed JSON using your own printing function (json_print)
            json_print(parsed_json);
        }
        else {
            fprintf(stderr, "JSON parsing failed\n");
        }
        json_free(parsed_json);
    }

    // Clean up
    free(chunk.memory);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return 0;
}
