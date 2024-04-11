#include "cpio.h"

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
    char buf[0x10] = {};

    memcpy(buf, (header)->c_magic, 6);
    printf("c_magic: %s" ENDL, buf);

    memcpy(buf, (header)->c_ino, 8);
    printf("c_ino: %s" ENDL, buf);

    memcpy(buf, (header)->c_mode, 8);
    printf("c_mode: %s" ENDL, buf);

    memcpy(buf, (header)->c_uid, 8);
    printf("c_uid: %s" ENDL, buf);

    memcpy(buf, (header)->c_gid, 8);
    printf("c_gid: %s" ENDL, buf);

    memcpy(buf, (header)->c_nlink, 8);
    printf("c_nlink: %s" ENDL, buf);

    memcpy(buf, (header)->c_mtime, 8);
    printf("c_mtime: %s" ENDL, buf);

    memcpy(buf, (header)->c_filesize, 8);
    printf("c_filesize: %s" ENDL, buf);

    memcpy(buf, (header)->c_devmajor, 8);
    printf("c_devmajor: %s" ENDL, buf);

    memcpy(buf, (header)->c_devminor, 8);
    printf("c_devminor: %s" ENDL, buf);

    memcpy(buf, (header)->c_rdevmajor, 8);
    printf("c_rdevmajor: %s" ENDL, buf);

    memcpy(buf, (header)->c_rdevminor, 8);
    printf("c_rdevminor: %s" ENDL, buf);

    memcpy(buf, (header)->c_namesize, 8);
    printf("c_namesize: %s" ENDL, buf);

    memcpy(buf, (header)->c_check, 8);
    printf("c_check: %s" ENDL, buf);
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
    char buf[0x100];
    memcpy(buf, file_name, name_size);
    printf("%s" ENDL, buf);
}

void cpio_cat_callback(char* param, cpio_newc_header* header, char* file_name, uint32_t name_size, char* file_data, uint32_t data_size) {
    // TODO: implement multi-parameter
    if (strcmp(param, file_name)) return;
    char buf[0x100];

    memcpy(buf, file_name, name_size);
    printf("Filename: %s" ENDL, buf);

    data_size %= 0x100;
    memcpy(buf, file_data, data_size);
    printf("%s" ENDL, buf);
}

void cpio_exec_callback(char* param, cpio_newc_header* header, char* file_name, uint32_t name_size, char* file_data, uint32_t data_size) {
    // TODO: implement multi-parameter
    if (strcmp(param, file_name)) return;
    exec(file_data, data_size);
}