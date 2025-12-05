#include "buffered_open.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

const char *filename = "Test3Output.txt";

int test1(){
    // ============================== TEST 1: Simple write to file ==================================
    const char *inputTest1 = "Test1";
    char readBuffer[1024] = {0};
    const char *expectedOutTest1 = "Test1";
    // Open file for writing
    buffered_file_t *bf = buffered_open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (!bf) {
        perror("buffered_open 1");
        return 1;
    }

    // Write text to the file
    if (buffered_write(bf, inputTest1, strlen(inputTest1)) == -1) {
        perror("buffered_write 1");
        buffered_close(bf);
        return 1;
    }

    // Flush the buffer
    if (buffered_flush(bf) == -1) {
        perror("buffered_flush 1");
        buffered_close(bf);
        return 1;
    }

    // Close the buffered file
    if (buffered_close(bf) == -1) {
        perror("buffered_close 1");
        return 1;
    }

    // Reopen file for reading with standard I/O to verify contents
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        perror("open 1");
        return 1;
    }
    ssize_t bytes_read = read(fd, readBuffer, sizeof(readBuffer) - 1);
    if (bytes_read == -1) {
        perror("read 1");
        close(fd);
        return 1;
    }
    readBuffer[bytes_read] = '\0';  // Null-terminate the string

    if (strcmp(readBuffer, expectedOutTest1) == 0) {
        printf("\033[0;32mTEST 1: PASSED\n\033[0m");
        return 0;
    } else {
        printf("\033[0;31mTEST 1: FAILED\n\033[0m");
        printf("\033[0;32mExpected output: %s \n\033[0m" , expectedOutTest1);
        printf("\033[0;31mActual output: %s \n\033[0m", readBuffer);
        return -1;
    }
    // ============================== END OF TEST 1: Simple write to file ==================================
}

int test2(){
    // ============================= TEST 2: Write to file with preappend flag =============================
    const char *inputTest2 = "Test2";
    char readBuffer[1024] = {0};
    const char *expectedOutTest2 = "Test2Test1";
    // Open file with preappend flag
    buffered_file_t *bf = buffered_open(filename, O_RDWR | O_PREAPPEND, 0);
    if (!bf) {
        perror("buffered_open 2");
        return 1;
    }
    if (buffered_write(bf, inputTest2, strlen(inputTest2)) == -1) {
        perror("buffered_write 2");
        buffered_close(bf);
        return 1;
    }
    if (buffered_flush(bf) == -1) {
        perror("buffered_flush 2");
        buffered_close(bf);
        return 1;
    }
    // Close the buffered file
    if (buffered_close(bf) == -1) {
        perror("buffered_close 2");
        return 1;
    }
    // Reopen file for reading with standard I/O to verify contents
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        perror("open 2");
        return 1;
    }
    ssize_t bytes_read = read(fd, readBuffer, sizeof(readBuffer) - 1);
    if (bytes_read == -1) {
        perror("read 2");
        close(fd);
        return 1;
    }
    readBuffer[bytes_read] = '\0';  // Null-terminate the string

    if (strcmp(readBuffer, expectedOutTest2) == 0) {
        printf("\033[0;32mTEST 2: PASSED\n\033[0m");
        return 0;
    } else {
        printf("\033[0;31mTEST 2: FAILED\n\033[0m");
        printf("\033[0;32mExpected output: %s \n\033[0m" , expectedOutTest2);
        printf("\033[0;31mActual output: %s \n\033[0m", readBuffer);
        return -1;
    }
    // ============================= END OF TEST 2: Write to file with preappend flag =============================
}
int test3(){
    // ============================= TEST 3: Write witout preappend flag =============================
    const char *inputTest3 = "Test3";
    char readBuffer[1024] = {0};
    const char *expectedOutTest3 = "Test2Test1Test3";
    // Open file without preappend flag
    buffered_file_t *bf = buffered_open(filename, O_RDWR | O_APPEND, 0);
    if (!bf) {
        perror("buffered_open 3");
        return 1;
    }
    if (buffered_write(bf, inputTest3, strlen(inputTest3)) == -1) {
        perror("buffered_write 3");
        buffered_close(bf);
        return 1;
    }
    if (buffered_flush(bf) == -1) {
        perror("buffered_flush 3");
        buffered_close(bf);
        return 1;
    }
    // Close the buffered file
    if (buffered_close(bf) == -1) {
        perror("buffered_close 3");
        return 1;
    }
    // Reopen file for reading with standard I/O to verify contents
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        perror("open 3");
        return 1;
    }
    ssize_t bytes_read = read(fd, readBuffer, sizeof(readBuffer) - 1);
    if (bytes_read == -1) {
        perror("read 3");
        close(fd);
        return 1;
    }
    readBuffer[bytes_read] = '\0';  // Null-terminate the string

    if (strcmp(readBuffer, expectedOutTest3) == 0) {
        printf("\033[0;32mTEST 3: PASSED\n\033[0m");
        return 0;
    } else {
        printf("\033[0;31mTEST 3: FAILED\n\033[0m");
        printf("\033[0;32mExpected output: %s \n\033[0m" , expectedOutTest3);
        printf("\033[0;31mActual output: %s \n\033[0m", readBuffer);
        return -1;
    }
    // ============================= END OF TEST 3: Write witout preappend flag ============================
}

int test4(){
    // ============================= TEST 4: Read first 4 bytes of file ====================================
    char readBuffer_file[1024] = {0};
    char readBuffer[1024] = {0};
    const char *expectedOutTest4_file = "Test2Test1Test3";
    const char *expectedOutTest4 = "Test";
    // Open file without preappend flag
    buffered_file_t *bf = buffered_open(filename, O_RDWR, 0);
    if (!bf) {
        perror("buffered_open 4");
        return 1;
    }
    ssize_t bytes_read;
    bytes_read = buffered_read(bf, readBuffer, 4);
    if (bytes_read == -1) {
        perror("buffered_read 4");
        buffered_close(bf);
        return 1;
    }

    // ======== TEST Empty flush ==========
    if (buffered_flush(bf) == -1) {
        perror("buffered_flush 4");
        buffered_close(bf);
        return 1;
    }
    // Reopen file for reading with standard I/O to verify contents
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        perror("open 4");
        return 1;
    }
    ssize_t bytes_read_file = read(fd, readBuffer_file, sizeof(readBuffer_file) - 1);
    if (bytes_read_file == -1) {
        perror("read 4");
        close(fd);
        return 1;
    }
    readBuffer_file[bytes_read_file] = '\0';  // Null-terminate the string

    if (strcmp(readBuffer_file, expectedOutTest4_file) != 0) {
        printf("\033[0;31mTEST 4: FAILED\n\033[0m");
        printf("\033[0;32mExpected output: %s \n\033[0m" , expectedOutTest4_file);
        printf("\033[0;31mActual output: %s \n\033[0m", readBuffer_file);
        return -1;
    }
    // Close the buffered file
    if (buffered_close(bf) == -1) {
        perror("buffered_close 4");
        return 1;
    }

    readBuffer[bytes_read] = '\0';  // Null-terminate the string

    if (strcmp(readBuffer, expectedOutTest4) == 0) {
        printf("\033[0;32mTEST 4: PASSED\n\033[0m");
        return 0;
    } else {
        printf("\033[0;31mTEST 4: FAILED\n\033[0m");
        printf("\033[0;32mExpected output: %s \n\033[0m" , expectedOutTest4);
        printf("\033[0;31mActual output: %s \n\033[0m", readBuffer);
        return -1;
    }
    // ============================= END OF TEST 4: Read first 4 bytes of file ====================================
}
int test5(){
    // =========================== TEST 5: Sequential Write to file with preappend flag ===========================
    const char *inputTest5 = "Test5";
    char readBuffer[1024] = {0};
    const char *expectedOutTest5_1 = "Test5Test5Test5Test2Test1Test3";
    // Open file with preappend flag
    buffered_file_t *bf = buffered_open(filename, O_RDWR | O_PREAPPEND, 0);
    if (!bf) {
        perror("buffered_open 5");
        return 1;
    }
    if (buffered_write(bf, inputTest5, strlen(inputTest5)) == -1) {
        perror("buffered_write 5");
        buffered_close(bf);
        return 1;
    }
    if (buffered_write(bf, inputTest5, strlen(inputTest5)) == -1) {
        perror("buffered_write 5");
        buffered_close(bf);
        return 1;
    }
    if (buffered_write(bf, inputTest5, strlen(inputTest5)) == -1) {
        perror("buffered_write 5");
        buffered_close(bf);
        return 1;
    }
    if (buffered_flush(bf) == -1) {
        perror("buffered_flush 5");
        buffered_close(bf);
        return 1;
    }
    // Close the buffered file
    if (buffered_close(bf) == -1) {
        perror("buffered_close 5");
        return 1;
    }
    // Reopen file for reading with standard I/O to verify contents
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        perror("open 5");
        return 1;
    }
    ssize_t bytes_read = read(fd, readBuffer, sizeof(readBuffer) - 1);
    if (bytes_read == -1) {
        perror("read 5");
        close(fd);
        return 1;
    }
    readBuffer[bytes_read] = '\0';  // Null-terminate the string

    if (strcmp(readBuffer, expectedOutTest5_1) != 0) {
        printf("\033[0;31mTEST 5: FAILED\n\033[0m");
        printf("\033[0;32mExpected output: %s \n\033[0m" , expectedOutTest5_1);
        printf("\033[0;31mActual output: %s \n\033[0m", readBuffer);
        return -1;
    }
    const char *expectedOutTest5_2 = "Test5Test5Test5Test2Test1Test3Test5Test5Test5Test5";
    bf = buffered_open(filename, O_RDWR | O_APPEND, 0);
    if (buffered_write(bf, inputTest5, strlen(inputTest5)) == -1) {
        perror("buffered_write 5");
        buffered_close(bf);
        return 1;
    }
    if (buffered_write(bf, inputTest5, strlen(inputTest5)) == -1) {
        perror("buffered_write 5");
        buffered_close(bf);
        return 1;
    }
    if (buffered_write(bf, inputTest5, strlen(inputTest5)) == -1) {
        perror("buffered_write 5");
        buffered_close(bf);
        return 1;
    }
    if (buffered_write(bf, inputTest5, strlen(inputTest5)) == -1) {
        perror("buffered_write 5");
        buffered_close(bf);
        return 1;
    }
    if (buffered_flush(bf) == -1) {
        perror("buffered_flush 5");
        buffered_close(bf);
        return 1;
    }
    // Close the buffered file
    if (buffered_close(bf) == -1) {
        perror("buffered_close 5");
        return 1;
    }
    // Reopen file for reading with standard I/O to verify contents
    fd = open(filename, O_RDWR);
    if (fd == -1) {
        perror("open 5");
        return 1;
    }
    bytes_read = read(fd, readBuffer, sizeof(readBuffer) - 1);
    if (bytes_read == -1) {
        perror("read 5");
        close(fd);
        return 1;
    }
    readBuffer[bytes_read] = '\0';  // Null-terminate the string

    if (strcmp(readBuffer, expectedOutTest5_2) == 0) {
        printf("\033[0;32mTEST 5: PASSED\n\033[0m");
        return 0;
    } else {
        printf("\033[0;31mTEST 5: FAILED\n\033[0m");
        printf("\033[0;32mExpected output: %s \n\033[0m" , expectedOutTest5_2);
        printf("\033[0;31mActual output: %s \n\033[0m", readBuffer);
        return -1;
    }
    // ======================= END OF TEST 5: Sequential Write to file with preappend flag ========================
}

int test6(){
    // ============================= TEST 6: Sequential Read to file ==============================================
    char readBuffer[1024] = {0};
    const char *expectedOutTest6 = "Test5Test5Test5Test2";
    buffered_file_t *bf = buffered_open(filename, O_RDWR, 0);
    if (!bf) {
        perror("buffered_open 6");
        return 1;
    }
    ssize_t bytes_read;
    bytes_read = buffered_read(bf, readBuffer, 5);
    if (bytes_read == -1) {
        perror("buffered_read 6");
        buffered_close(bf);
        return 1;
    }
    bytes_read = buffered_read(bf, readBuffer + 5, 5);
    if (bytes_read == -1) {
        perror("buffered_read 6");
        buffered_close(bf);
        return 1;
    }
    bytes_read = buffered_read(bf, readBuffer + 10, 5);
    if (bytes_read == -1) {
        perror("buffered_read 6");
        buffered_close(bf);
        return 1;
    }
    bytes_read = buffered_read(bf, readBuffer + 15, 5);
    if (bytes_read == -1) {
        perror("buffered_read 6");
        buffered_close(bf);
        return 1;
    }
    if (buffered_flush(bf) == -1) {
        perror("buffered_flush 6");
        buffered_close(bf);
        return 1;
    }
    // Close the buffered file
    if (buffered_close(bf) == -1) {
        perror("buffered_close 6");
        return 1;
    }
    readBuffer[20] = '\0';  // Null-terminate the string
    if (strcmp(readBuffer, expectedOutTest6) == 0) {
        printf("\033[0;32mTEST 6: PASSED\n\033[0m");
        return 0;
    } else {
        printf("\033[0;31mTEST 6: FAILED\n\033[0m");
        printf("\033[0;32mExpected output: %s \n\033[0m" , expectedOutTest6);
        printf("\033[0;31mActual output: %s \n\033[0m", readBuffer);
        return -1;
    }
    // ============================= END OF TEST 6: Sequential Read to file ==============================================
}


int main() {
    int countTestPassed = 0;
    if (test1() == 0){
        countTestPassed++;
    }
    if (test2() == 0){
        countTestPassed++;
    }
    if (test3() == 0){
        countTestPassed++;
    }
    if (test4() == 0){
        countTestPassed++;
    }
    if (test5() == 0){
        countTestPassed++;
    }
    if(test6() == 0){
        countTestPassed++;
    }
    if (countTestPassed == 6){
        printf("\033[0;32mAll tests passed!\n\033[0m");
    } else {
        printf("\033[0;31mSome tests failed\n\033[0m");
    }
    return 0;
}