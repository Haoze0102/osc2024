#include "cpio.h"
#include "exec.h"


cpio_newc_header* header;

void cpio_init() {
    header = malloc(sizeof(cpio_newc_parse_header) / sizeof(char));
}

void cpio_newc_parser(void* callback, char* param) {
    char *cpio_ptr = INITRD_ADDR,
         *file_name, *file_data;
    uint32_t name_size, data_size;
    cpio_newc_header* header = malloc(CPIO_NEWC_HEADER_SIZE);

    while (1) {
        cpio_newc_parse_header(&cpio_ptr, &header);
        // cpio_newc_show_header(header);

        name_size = hex_ascii_to_uint32(header->c_namesize, sizeof(header->c_namesize) / sizeof(char));
        data_size = hex_ascii_to_uint32(header->c_filesize, sizeof(header->c_filesize) / sizeof(char));

        cpio_newc_parse_data(&cpio_ptr, &file_name, name_size, CPIO_NEWC_HEADER_SIZE);

        cpio_newc_parse_data(&cpio_ptr, &file_data, data_size, 0);

        if (strcmp(file_name, "TRAILER!!!") == 0) {
            break;
        }
        ((void (*)(char* param, cpio_newc_header* header, char* file_name, uint32_t name_sizez, char* file_data, uint32_t data_size))callback)(
            param, header, file_name, name_size, file_data, data_size);
    }
}

void cpio_newc_parse_header(char** cpio_ptr, cpio_newc_header** header) {
    *header = *cpio_ptr;
    *cpio_ptr += sizeof(cpio_newc_header) / sizeof(char);
}

void cpio_newc_show_header(cpio_newc_header* header) {
    printf("c_magic: %.*s" ENDL, 6, (header)->c_magic);
    printf("c_ino: %.*s" ENDL, 8, (header)->c_ino);
    printf("c_mode: %.*s" ENDL, 8, (header)->c_mode);
    printf("c_uid: %.*s" ENDL, 8, (header)->c_uid);
    printf("c_gid: %.*s" ENDL, 8, (header)->c_gid);
    printf("c_nlink: %.*s" ENDL, 8, (header)->c_nlink);
    printf("c_mtime: %.*s" ENDL, 8, (header)->c_mtime);
    printf("c_filesize: %.*s" ENDL, 8, (header)->c_filesize);
    printf("c_devmajor: %.*s" ENDL, 8, (header)->c_devmajor);
    printf("c_devminor: %.*s" ENDL, 8, (header)->c_devminor);
    printf("c_rdevmajor: %.*s" ENDL, 8, (header)->c_rdevmajor);
    printf("c_rdevminor: %.*s" ENDL, 8, (header)->c_rdevminor);
    printf("c_namesize: %.*s" ENDL, 8, (header)->c_namesize);
    printf("c_check: %.*s" ENDL, 8, (header)->c_check);
}

void cpio_newc_parse_data(char** cpio_ptr, char** buf, uint32_t size, uint32_t offset) {
    *buf = *cpio_ptr;
    while ((size + offset) % 4 != 0) {
        size += 1;
    }
    *cpio_ptr += size;
}

void cpio_ls_callback(char* param, cpio_newc_header* header, char* file_name, uint32_t name_size, char* file_data, uint32_t data_size) {
    // TODO: impelment parameter
    printf("%.*s" ENDL, name_size, file_name);
}

void cpio_cat_callback(char* param, cpio_newc_header* header, char* file_name, uint32_t name_size, char* file_data, uint32_t data_size) {
    // TODO: implement multi-parameter
    if (strcmp(param, file_name)) return;
    printf("Filename: %.*s" ENDL, name_size, file_name);
    printf("%.*s" ENDL, data_size, file_data);
}

void cpio_exec_callback(char* param, cpio_newc_header* header, char* file_name, uint32_t name_size, char* file_data, uint32_t data_size) {
    // TODO: implement multi-parameter
    if (strcmp(param, file_name)) return;
    exec(file_data, data_size);
}