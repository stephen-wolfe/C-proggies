#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>


/* This program tests dup(). It opens a text file called /tmp/file.txt,
 * duplicates its fd, writes to it using both file descriptors, and
 * prints the file descriptor offsets as it goes along to show that 
 * the descriptors point to the same entries in the kernel's file
 * table, as per the dup man page. It then uses various ways to look
 * at the data written to the file and shows what happens when a
 * printf function hits a file hole.
 * This is a mix of exercise 5-4 and 5-5 in tlpi.
 * */

int print_all_characters_in_file(int fd) {
    char newchar; 
    int byte_read;
    
    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1) {
        fprintf(stderr, "lseek");
        return -1;
    }

    printf("File contains %ld bytes.\n", file_size);
    
    if (lseek(fd, 0, SEEK_SET) == -1) {
        fprintf(stderr, "lseek");
        return -1;
    }
    
    printf("Printing all characters in file:\n\n##############\n\n");
    for (int i=0; i<file_size; i++) {
        byte_read = read(fd, &newchar, 1); 
        if (byte_read == -1) {
            fprintf(stderr, "read");
            return -1; 
        } 
        if isprint(newchar) {
            printf("%c", newchar);
        } else {
            printf(" ");
        }
    }
    printf("\n\n##############\n\n");
}

int print_all_characters_in_buffer(char *buffer, size_t buffer_size) {
    for (int i=0; i<buffer_size; i++) {
        if (isprint(buffer[i])) { 
            printf("%c", buffer[i]);
        } else {
            printf(" ");
        }
    }
}

int check_offsets(int fd1, int fd2) {
    off_t off1 = lseek(fd1, 0, SEEK_CUR);
    off_t off2 = lseek(fd2, 0, SEEK_CUR);
    if ((off1 == -1) || (off2 == -1)) {
        fprintf(stderr, "lseek");
        return -1;
    } 

    printf("\noffset1: %ld, offset2: %ld\n", off1, off2); 
    
    if (off1 != off2) {
        printf("File offsets for fd1 and fd2 don't match.\n");
        return 1;
    } else {
        printf("File offsets for fd1 and fd2 are the same\n");
        return 0;
    } 
}

int check_flags(int fd1, int fd2) {
    int flags1 = fcntl(fd1, F_GETFL);
    int flags2 = fcntl(fd2, F_GETFL);
    
    if ((flags1 == -1) || (flags2 == -1)) {
        fprintf(stderr, "fcntl");
        return -1; 
    }

    if (flags1 != flags2) {
        printf("\nOpen file access flags for fd1 and fd2 don't match.\n"); 
        return 1;
    }
    else {
        printf("\nOpen file access flags for fd1 and fd2 are the same.\n");
        return 0;
    }
}


int main(int argc, char *argv[]) {

    int newfd1 = open("/tmp/file.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (newfd1 == -1) {
        perror("open newfd1");
        exit(EXIT_FAILURE);
    }    

    printf("\nnewfd1 = %d\n", newfd1);
    int newfd2 = dup(newfd1);
    if (newfd2 == -1) {
        perror("dup"); 
        close(newfd1);
        exit(EXIT_FAILURE);
    }        

    printf("newfd2, which points to same open file description as newfd1: %d\n", newfd2);

    if (check_flags(newfd1, newfd2) == -1) {
        fprintf(stderr, "check_flags returned error");
        close(newfd1);
        close(newfd2);
        exit(EXIT_FAILURE);
    }
    
    if (check_offsets(newfd1, newfd2) == -1) {
        close(newfd1);
        close(newfd2);
        fprintf(stderr, "File offsets don't match. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    int bytes_written = write(newfd1, "aaaaa", 5); 
    if (bytes_written == -1) {
        perror("write");
        close(newfd1);
        close(newfd2);
        exit(EXIT_FAILURE);
    }

    printf("\nChecking offsets after writing 5 characters to newfd1\n");

    if (check_offsets(newfd1, newfd2) == -1) {
        close(newfd1);
        close(newfd2);
        fprintf(stderr, "File offsets don't match. Exiting.\n");
        exit(EXIT_FAILURE);
    }
     
    bytes_written = write(newfd2, "bbbbb", 7);     
    if (bytes_written == -1) {
        perror("write");
        close(newfd1);
        close(newfd2);
        exit(EXIT_FAILURE);
    } 
   
    printf("\nChecking offsets after writing 7 characters to newfd2\n");
    printf("File holes shouldn't matter for the offsets.\n");
    if (check_offsets(newfd1, newfd2) == -1) {
        close(newfd1);
        close(newfd2);
        fprintf(stderr, "File offsets don't match. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    bytes_written = write(newfd2, "ccc", 3);     
    if (bytes_written == -1) {
        perror("write");
        close(newfd1);
        close(newfd2);
        exit(EXIT_FAILURE);
    } 
    
    printf("\nChecking offsets after writing 7 characters to newfd2\n");
    
    if (check_offsets(newfd1, newfd2) == -1) {
        close(newfd1);
        close(newfd2);
        fprintf(stderr, "File offsets don't match. Exiting.\n");
        exit(EXIT_FAILURE);
    }





    off_t beginning = lseek(newfd1, 0, SEEK_SET);
    if (beginning == -1) {
        perror("lseek"); 
        close(newfd1);
        close(newfd2);
        exit(EXIT_FAILURE);
    }
         
    char filebuf[50];
     
    int bytes_read = read(newfd1, filebuf, sizeof(filebuf));
    if (bytes_read == -1) {
        perror("read");
        close(newfd1);
        close(newfd2);
        exit(EXIT_FAILURE);
    } 

    if (check_offsets(newfd1, newfd2) == -1) {
        close(newfd1);
        close(newfd2);
        fprintf(stderr, "File offsets don't match. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    printf("\nWriting buffer read from file:\n\n###############\n\n");
    
    printf("%s", filebuf);
    
    printf("\n\n##############\n\n");
    
    printf("sizeof(filebuf) = %ld. strlen(filebuf) = %ld\n", sizeof(filebuf), strlen(filebuf));
    printf("printf() and strlen() functions stopped at the file hole, but the read() function didn't.\n");
    printf("\nPrinting all the characters in the 50 character buffer:\n\n##############\n\n");
    
    if (print_all_characters_in_buffer(filebuf, sizeof(filebuf)) == -1 ) {
        fprintf(stderr, "Error printing all characters in buffer");
        close(newfd1);
        close(newfd2);
        exit(EXIT_FAILURE);
    }
    printf("\n\n##############\n\n");

    if (print_all_characters_in_file(newfd1) == -1) {
        fprintf(stderr, "Error printing all characters in file");
        close(newfd1);
        close(newfd2);
        exit(EXIT_FAILURE);
    }
   
    errno = 0;
    if (close(newfd2) == -1) {
        perror("close newfd2");
        close(newfd1); 
        exit(EXIT_FAILURE);
    }

    errno = 0; 
    if (close(newfd1) == -1) {
        perror("close newfd1");
        exit(EXIT_FAILURE);
    }

    printf("We're golden.\n");

    return EXIT_SUCCESS;
}
