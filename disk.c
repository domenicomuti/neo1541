#include "disk.h"

char disk_path[PATH_MAX];
struct vic_disk_info disk_info;
extern vic_string data_buffer;
extern vic_string filename;

unsigned short get_file_blocks(char *path, char *_filename, vic_size *filesize) {
    char *_path = calloc(strlen(path) + strlen(_filename) + 2, sizeof(char));
    if (_path == NULL) {
        fprintf(stderr, "ERROR: Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    strcpy(_path, path);
    int offset = 0;
    if (_path[strlen(path) - 1] != FILESEPARATOR) {
        _path[strlen(path)] = FILESEPARATOR;
        offset = 1;
    }
    strcpy(_path + strlen(path) + offset, _filename);

#ifdef __linux__
    unsigned long long blocks;
    FILE* fptr = fopen(_path, "rb");
    if (fptr == NULL) {
        blocks = 0;
        *filesize = 0;
    }
    else {
        fseek(fptr, 0L, SEEK_END);
        *filesize = ftell(fptr);
        if (*filesize == LONG_MAX) *filesize = 0;
        blocks = ceil(*filesize / 256.0);
        fclose(fptr);
    }
#elif _WIN64
    unsigned long long blocks;
    int fh = _open(_path, _O_BINARY | _O_RDONLY);
    if (fh == -1) {
        blocks = 0;
    }
    else {
        _lseeki64(fh, 0L, SEEK_END);
        blocks = ceil(_telli64(fh) / 256.0);
        _close(fh);
    }
#elif _WIN32
    // TODO
#endif

    free(_path);
    if (blocks > 999) blocks = 999;
    return (unsigned short)blocks;
}

void get_disk_info() {
    DIR *dir;
    struct dirent *entry;
    
    dir = opendir(disk_path);
    if (dir) {
        disk_info.type = DISK_DIR;
        disk_info.blocks_free = 0;

        // Header
        memset(disk_info.header, ' ', HEADER_SIZE);
        disk_info.header[0] = 0x12; // Reverse print
        disk_info.header[1] = '\"';
        char *_header = (char *)(disk_info.header + 2);
        char *_basename = (char *)basename((unsigned char *)disk_path);
        int _basename_len = strlen(_basename);
        if (_basename_len > (HEADER_SIZE - 9))
            _basename_len = HEADER_SIZE - 9;
        memcpy(_header, _basename, _basename_len);
        strtolower(_header, _basename_len);
        for (int i = 0; i < _basename_len; i++) _header[i] = a2p(_header[i]);
        memcpy(disk_info.header + HEADER_SIZE - 7, (unsigned char *)"\" ID 00", 7);

        struct file_hash_entry {
            char filename[FILENAMEMAXSIZE + 1];
            int n;
            UT_hash_handle hh;
        };
        struct file_hash_entry *files = NULL;

        disk_info.n_dir = 0;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0) continue;

            vic_disk_dir *vic_dir = &disk_info.dir[disk_info.n_dir++];
            strcpy(vic_dir->filename_local, entry->d_name);

            memset(vic_dir->filename, ' ', FILENAMEMAXSIZE + 2);
            vic_dir->filename[0] = '\"';

            char *_filename = (char *)(vic_dir->filename + 1);
            int d_namelen = strlen(entry->d_name);
            if (d_namelen > FILENAMEMAXSIZE)
                d_namelen = FILENAMEMAXSIZE;
            memcpy(_filename, entry->d_name, d_namelen);
            strtolower(_filename, d_namelen);
            for (int i = 0; i < d_namelen; i++) _filename[i] = a2p(_filename[i]);

            _filename[d_namelen] = '\0';
            struct file_hash_entry *f;
            HASH_FIND_STR(files, _filename, f);
            if (f == NULL) {
                f = malloc(sizeof *f);
                if (f == NULL) {
                    fprintf(stderr, "ERROR: Memory allocation error\n");
                    exit(EXIT_FAILURE);
                }
                strcpy(f->filename, _filename);
                f->n = 1;
                HASH_ADD_STR(files, filename, f);
            }
            else {
                f->n++;
            }
            if (f->n > 1) {
                if (f->n > 0xFFFF)
                    f->n = 0xFFFF; // Max 65535 name collisions, more then anybody really needs
                char _t[6];
                sprintf(_t, "#%X", f->n);
                int _t_len = strlen(_t);
                if ((d_namelen + _t_len) > FILENAMEMAXSIZE) 
                    d_namelen = FILENAMEMAXSIZE - _t_len;
                    
                memcpy(_filename + d_namelen, _t, _t_len);
                d_namelen += _t_len;
            }
            _filename[d_namelen] = '\"';
            vic_dir->filename_length = d_namelen;

            memcpy(vic_dir->type, (unsigned char *)" PRG ", 5);
            vic_dir->blocks = get_file_blocks(disk_path, entry->d_name, &vic_dir->filesize);
        }
        closedir(dir);

        struct file_hash_entry *current_file, *tmp;
        HASH_ITER(hh, files, current_file, tmp) {
            HASH_DEL(files, current_file);
            free(current_file);
        }
    }
    else {
        char ext[4] = {0};
        substr(ext, disk_path, -3, 3);
        strtolower(ext, 0);

        if (strcmp(ext, "d64") == 0 || strcmp(ext, "d71") == 0 || strcmp(ext, "d81") == 0) { // TODO: test d71 and d81 images
            disk_info.type = DISK_IMAGE;
            cc1541(CC1541_PRINT_DIRECTORY);
        }
    }   
}

void extract_file_from_image() {
    data_buffer.length = 0;
    cc1541(CC1541_EXTRACT_FILES);
}

void save_file_to_image() {
    cc1541(CC1541_SAVE_FILE_TO_IMAGE);
}

void directory_listing() {
#if DEBUG
    printf("%s\n", __func__);
#endif
    data_buffer.length = filename.length = 0;

    int i_memory = 0x1001;
    int i_memory_start = i_memory - 2;
    int i_next_line;

    data_buffer.string[data_buffer.length++] = i_memory & 0x00FF; // Start
    data_buffer.string[data_buffer.length++] = (i_memory & 0xFF00) >> 8;
    i_next_line = data_buffer.length;
    
    data_buffer.string[4] = 0x00; // Line number 0
    data_buffer.string[5] = 0x00;
    data_buffer.length += 4;

    memcpy(data_buffer.string + data_buffer.length, disk_info.header, HEADER_SIZE);
    data_buffer.length += HEADER_SIZE;

    data_buffer.string[data_buffer.length] = 0x00; // new line
    data_buffer.length++;
    
    i_memory = i_memory_start + data_buffer.length;
    data_buffer.string[i_next_line] = i_memory & 0x00FF;
    data_buffer.string[i_next_line + 1] = (i_memory & 0xFF00) >> 8;

    for (int i = 0; i < disk_info.n_dir; i++) {
        i_next_line = data_buffer.length;
        
        data_buffer.length += 2;
        data_buffer.string[data_buffer.length++] = disk_info.dir[i].blocks & 0x00FF;
        data_buffer.string[data_buffer.length++] = (disk_info.dir[i].blocks & 0xFF00) >> 8;

        int start_offset;
        if (disk_info.dir[i].blocks < 10) start_offset = 3;
        else if (disk_info.dir[i].blocks < 100) start_offset = 2;
        else if (disk_info.dir[i].blocks < 1000) start_offset = 1;
        for (int j = 0; j < start_offset; j++) {
            data_buffer.string[j + data_buffer.length] = ' ';
        }
        data_buffer.length += start_offset;

        memcpy(data_buffer.string + data_buffer.length, disk_info.dir[i].filename, FILENAMEMAXSIZE + 2);
        data_buffer.length += FILENAMEMAXSIZE + 2;

        memcpy(data_buffer.string + data_buffer.length, disk_info.dir[i].type, 5);
        data_buffer.length += 5;

        data_buffer.string[data_buffer.length] = 0x00; // new line
        data_buffer.length++;

        i_memory = i_memory_start + data_buffer.length;
        data_buffer.string[i_next_line] = i_memory & 0x00FF;
        data_buffer.string[i_next_line + 1] = (i_memory & 0xFF00) >> 8;
    }

    i_next_line = data_buffer.length;

    data_buffer.length += 2;
    data_buffer.string[data_buffer.length++] = disk_info.blocks_free & 0x00FF; // Blocks free
    data_buffer.string[data_buffer.length++] = (disk_info.blocks_free & 0xFF00) >> 8;

    memcpy(data_buffer.string + data_buffer.length, (unsigned char*)"BLOCKS FREE.              ", 26);
    data_buffer.length += 26;
    data_buffer.string[data_buffer.length++] = 0;

    i_memory = i_memory_start + data_buffer.length;
    data_buffer.string[i_next_line] = i_memory & 0x00FF;
    data_buffer.string[i_next_line + 1] = (i_memory & 0xFF00) >> 8;

    data_buffer.string[data_buffer.length++] = 0;
    data_buffer.string[data_buffer.length++] = 0;

    // TODO: BAM MESSAGE

    /*FILE* fptr = fopen("image_examples/test_dir.prg", "wb");
    fwrite(data_buffer, data_buffer.length, 1, fptr);
    fclose(fptr);*/
}

void write_data_buffer() {
#if DEBUG
    printf("%s\n", __func__);
#endif
    data_buffer.length = 0;

    get_disk_info();

    if (vic_string_equal_string(&filename, "$")) {
        printf("[%ld] %sLIST FILES:%s %s\n", get_microsec(), COLOR_YELLOW, COLOR_RESET, disk_path);
        directory_listing();
    }
    else if (vic_string_equal_string(&filename, "..")) {
        char *t = strrchr(disk_path, FILESEPARATOR);
        
        if (t != &disk_path[0]) {
            t[0] = '\0';
        }
        else {
            // TODO: WIN32 VERSION
            t[0] = FILESEPARATOR;
            t[1] = '\0';
        }

        filename.string[0] = '$';
        filename.length = 1;
        write_data_buffer();
    }
    else if (disk_info.type == DISK_IMAGE) {
        char _filename[NAME_MAX + 1];
        for (int i = 0; i < filename.length; i++) _filename[i] = p2a(filename.string[i]);
        _filename[filename.length] = '\0';
        printf("[%ld] %sSENDING FILE:%s %s -> %s\n", get_microsec(), COLOR_YELLOW, COLOR_RESET, disk_path, _filename);
        extract_file_from_image();
        if (data_buffer.length == 0)
            return;
    }
    else if (disk_info.type == DISK_DIR) {
        char _disk_path[PATH_MAX];
        strcpy(_disk_path, disk_path);

        int _disk_path_len = strlen(_disk_path);
        if (_disk_path_len > 1)
            _disk_path[_disk_path_len++] = FILESEPARATOR;

        vic_disk_dir *dir_entry = NULL;
        for (int i = 0; i < disk_info.n_dir; i++) {
            vic_string _filename = {
                .string = disk_info.dir[i].filename + 1,
                .length = disk_info.dir[i].filename_length
            };
            if (vic_string_equal_vic_string(&filename, &_filename)) {
                dir_entry = &disk_info.dir[i];
                break;
            }
        }
        if (dir_entry == NULL)
            return;
        
        strcpy(_disk_path + _disk_path_len, dir_entry->filename_local);
        _disk_path[_disk_path_len + filename.length] = '\0';

        DIR *dir;
        dir = opendir(_disk_path);
        char ext[4] = {0};
        substr(ext, _disk_path, -3, 3);
        strtolower(ext, 0);

        if (dir || strcmp(ext, "d64") == 0 || strcmp(ext, "d71") == 0 || strcmp(ext, "d81") == 0) {
            strcpy(disk_path, _disk_path);
            filename.string[0] = '$';
            filename.length = 1;
            write_data_buffer();
        }
        else {
            FILE *fptr = fopen(_disk_path, "rb");
            if (fptr == NULL)
                return;
            data_buffer.length = dir_entry->filesize;
            fread(data_buffer.string, data_buffer.length, 1, fptr);
            printf("[%ld] %sSENDING FILE:%s %s (%ld bytes)\n", get_microsec(), COLOR_YELLOW, COLOR_RESET, _disk_path, data_buffer.length);
            fclose(fptr);
        }
    }
}