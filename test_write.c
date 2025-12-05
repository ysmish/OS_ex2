#include "buffered_open.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define TEST_FILE "test_output.txt"
#define TEST_PASS 0
#define TEST_FAIL 1

// Helper function to verify the content of the file
int verify_file_content(const char *expected_content) {
    FILE *fp = fopen(TEST_FILE, "r");
    if (!fp) {
        perror("Error opening test file for verification");
        return TEST_FAIL;
    }

    printf("\n--- Verifying File Content ---\n");
    int i = 0;
    int ch;
    int matches = TEST_PASS;
    while ((ch = fgetc(fp)) != EOF) {
        if (i < strlen(expected_content)) {
            printf("Read: '%c' | Expected: '%c'\n", ch, expected_content[i]);
            if (ch != expected_content[i]) {
                printf("Verification FAILED at index %d.\n", i);
                matches = TEST_FAIL;
                break;
            }
        } else {
            printf("Verification FAILED: File content is longer than expected.\n");
            matches = TEST_FAIL;
            break;
        }
        i++;
    }

    if (i < strlen(expected_content) && matches == TEST_PASS) {
        printf("Verification FAILED: File content is shorter than expected (Read %d bytes, Expected %zu bytes).\n", i, strlen(expected_content));
        matches = TEST_FAIL;
    } else if (matches == TEST_PASS) {
        printf("Verification SUCCESS: File contents match the expected total (%zu bytes).\n", strlen(expected_content));
    }

    fclose(fp);
    return matches;
}

int main() {
    printf("--- Starting buffered_write tests ---\n");

    // Clear previous file content
    remove(TEST_FILE); 

    // Test 1: Write small data (should stay in buffer)
    const char *test1_data = "AAAABBBBCCCC"; // 12 bytes
    printf("\nTEST 1: Writing %zu bytes (should buffer).\n", strlen(test1_data));
    
    buffered_file_t *bf = buffered_open(TEST_FILE, O_WRONLY | O_CREAT, 0644);
    if (!bf) return TEST_FAIL;

    if (buffered_write(bf, test1_data, strlen(test1_data)) == -1) {
        perror("TEST 1 buffered_write failed");
        buffered_close(bf);
        return TEST_FAIL;
    }
    printf("TEST 1: Successfully buffered. Closing to force flush...\n");

    // Close forces flush
    if (buffered_close(bf) == -1) {
        perror("TEST 1 buffered_close failed");
        return TEST_FAIL;
    }

    if (verify_file_content(test1_data) == TEST_FAIL) return TEST_FAIL;

    // Test 2: Write large data (requires multiple flushes)
    remove(TEST_FILE);
    printf("\nTEST 2: Writing %d bytes (requires multiple auto-flushes).\n", BUFFER_SIZE * 3 + 1);
    
    // Create large pattern: "0123456789..."
    char *large_data = (char *)malloc(BUFFER_SIZE * 3 + 1);
    if (!large_data) {
        perror("malloc failed for large data");
        return TEST_FAIL;
    }
    size_t large_size = BUFFER_SIZE * 3 + 1;
    for (size_t i = 0; i < large_size; i++) {
        large_data[i] = '0' + (i % 10);
    }
    large_data[large_size - 1] = 'X'; // Special char at the end

    bf = buffered_open(TEST_FILE, O_WRONLY | O_CREAT, 0644);
    if (!bf) { free(large_data); return TEST_FAIL; }

    if (buffered_write(bf, large_data, large_size) == -1) {
        perror("TEST 2 buffered_write failed");
        buffered_close(bf);
        free(large_data);
        return TEST_FAIL;
    }
    printf("TEST 2: Successfully wrote %zu bytes. Closing...\n", large_size);

    if (buffered_close(bf) == -1) {
        perror("TEST 2 buffered_close failed");
        free(large_data);
        return TEST_FAIL;
    }
    
    int result = verify_file_content(large_data);
    free(large_data);
    
    if (result == TEST_FAIL) return TEST_FAIL;


    // Test 3: Multiple small writes followed by flush (ensures no data loss between calls)
    remove(TEST_FILE);
    const char *part_a = "Part A ";
    const char *part_b = "Part B ";
    const char *part_c = "Part C";
    const char *expected_3 = "Part A Part B Part C";
    printf("\nTEST 3: Multiple small writes, closing for final flush.\n");
    
    bf = buffered_open(TEST_FILE, O_WRONLY | O_CREAT, 0644);
    if (!bf) return TEST_FAIL;

    if (buffered_write(bf, part_a, strlen(part_a)) == -1) goto fail_t3;
    if (buffered_write(bf, part_b, strlen(part_b)) == -1) goto fail_t3;
    if (buffered_write(bf, part_c, strlen(part_c)) == -1) goto fail_t3;
    
    printf("TEST 3: Successfully buffered all parts. Closing...\n");
    if (buffered_close(bf) == -1) {
        perror("TEST 3 buffered_close failed");
        return TEST_FAIL;
    }

    if (verify_file_content(expected_3) == TEST_FAIL) return TEST_FAIL;

    printf("\n*** All buffered_write tests passed! ***\n");
    return TEST_PASS;

fail_t3:
    perror("TEST 3 buffered_write failed");
    buffered_close(bf);
    return TEST_FAIL;
}