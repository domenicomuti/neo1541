#include "device.h"

extern int addr;

vic_byte last_command;

int device_resetted = 1;
int device_attentioned = 0;
int device_listening = 0;
int device_talking = 0;
int open_mode;

extern char disk_path[PATH_MAX + 1];
extern struct vic_disk_info disk_info;

vic_string data_buffer;
vic_string filename;

#ifdef _WIN64
    extern LARGE_INTEGER lpFrequency;
#endif

void initialize_buffers() {
    vic_byte *_data_buffer = malloc(MAX_DATA_BUFFER_SIZE * sizeof(vic_byte));
    if (_data_buffer == NULL) {
        fprintf(stderr, "ERROR: Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    data_buffer.string = _data_buffer;
    data_buffer.length = 0;

    vic_byte *_filename = malloc(FILENAMEMAXSIZE * sizeof(vic_byte));
    if (_filename == NULL) {
        fprintf(stderr, "ERROR: Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    filename.string = _filename;
    filename.length = 0;
}

void free_buffers() {
    free(data_buffer.string);
    free(filename.string);
}

void reset_device() {
    device_resetted = device_attentioned = device_listening = device_talking = 0;
}

vic_byte get_byte() {
    vic_byte byte = 0;

    //printf("START BYTE TRANSMISSION\n");

    //suseconds_t a = get_microsec();

    // bit 1
    wait_clock(0);
    byte = (INB(addr+1) & 0x40) >> 6;
    wait_clock(1);

    // bit 2
    wait_clock(0);
    byte |= (INB(addr+1) & 0x40) >> 5;
    wait_clock(1);

    // bit 3
    wait_clock(0);
    byte |= (INB(addr+1) & 0x40) >> 4;
    wait_clock(1);

    // bit 4
    wait_clock(0);
    byte |= (INB(addr+1) & 0x40) >> 3;
    wait_clock(1);

    // bit 5
    wait_clock(0);
    byte |= (INB(addr+1) & 0x40) >> 2;
    wait_clock(1);
    
    // bit 6
    wait_clock(0);
    byte |= (INB(addr+1) & 0x40) >> 1;
    wait_clock(1);

    // bit 7
    wait_clock(0);
    byte |= (INB(addr+1) & 0x40);
    wait_clock(1);

    // bit 8
    wait_clock(0);
    byte |= (INB(addr+1) & 0x40) << 1;
    wait_clock(1);

    //printf("%ld\n", get_microsec() - a);
    
    //printf("END BYTE TRANSMISSION\n");

    return byte;
}

void handle_atn() {
    printf("%sATN ON\n%s", COLOR_CYAN, COLOR_RESET);
            
    set_data(1);
    wait_clock(1);

    do {
        wait_clock(0); // TALKER IS READY TO SEND
        set_data(0);   // LISTENER IS READY FOR DATA

        wait_clock(1);
        vic_byte command = get_byte();
        last_command = command;

        print_command_name(command);

        //printf("BYTE: %c - %X\n", command, command);

        if ((command & 0xF0) == LISTEN) {
            device_attentioned = device_listening = (command & 0x0F) == DEVICE;
            device_talking = 0;
        }
        else if((command & 0xF0) == UNLISTEN) {
            device_listening = 0;
        }
        else if ((command & 0xF0) == OPEN) {
            open_mode = command & 0x0F;
        }
        else if ((command & 0xF0) == TALK) {
            device_listening = 0;
            device_attentioned = device_talking = (command & 0x0F) == DEVICE;
        }
        else if ((command & 0xF0) == UNTALK) {
            device_talking = 0;
        }
        else if ((command & 0xF0) == OPEN_CHANNEL) {
            //
        }
        else if ((command & 0xF0) == CLOSE) {
            //
        }
        else {
            device_attentioned = device_listening = device_talking = 0;
        }

        if (!device_attentioned) {
            wait_atn(0);
            continue;
        }

        set_data(1); // Frame Handshake
        wait_clock(0);

        if ((command & 0xF0) == UNLISTEN) {
            device_attentioned = 0;
        }
        else if ((command & 0xF0) == UNTALK) {
            device_attentioned = 0;
        }
    }
    while (atn(1) && !device_resetted);

    printf("%sATN OFF\n%s", COLOR_CYAN, COLOR_RESET);
}

void read_bytes() {
    printf("%sGET DATA ON\n%s", COLOR_MAGENTA, COLOR_RESET);

    data_buffer.length = 0;

    do {
        wait_clock(0); // TALKER IS READY TO SEND
        set_data(0);   // LISTENER IS READY FOR DATA

        int _eoi = 0;
        if (eoi()) {
            set_data(1);
            microsleep(60);
            set_data(0);
            _eoi = 1;
        }

        wait_clock(1);
        data_buffer.string[data_buffer.length] = get_byte();
        printf("%s%c - 0x%X\n%s", COLOR_YELLOW, data_buffer.string[data_buffer.length], data_buffer.string[data_buffer.length], COLOR_RESET);

        data_buffer.length++;

        set_data(1); // Frame Handshake

        microsleep(100);

        if (_eoi) break;
    }
    while (1);

    /*FILE *fptr = fopen("/home/noelyoung/list.prg", "a");
    fwrite(data_buffer, data_buffer.length, 1, fptr);
    fclose(fptr);*/

    if ((last_command & 0xF0) == OPEN) {
        for (int i = 0; i < data_buffer.length; i++) {
            filename.string[i] = data_buffer.string[i];
        }
        filename.length = data_buffer.length;
        data_buffer.length = 0;
    }

    device_listening = 0;

    printf("%sGET DATA OFF\n%s", COLOR_MAGENTA, COLOR_RESET);
}

void directory_listing() {
    data_buffer.length = filename.length = 0;
    //for (int i = 0; i < MAX_DATA_BUFFER_SIZE; i++) data_buffer.string[i] = 0;

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

void load_file() {
    data_buffer.length = 0;

    get_disk_info();

    if (vic_string_equal_string(&filename, "$")) {
        printf("List files: %s\n", disk_path);
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

        printf("Go to parent directory\n");
        filename.string[0] = '$';
        filename.length = 1;
        load_file();
    }
    else if (disk_info.type == DISK_IMAGE) {
        char _filename[NAME_MAX + 1];
        for (int i = 0; i < filename.length; i++) _filename[i] = p2a(filename.string[i]);
        _filename[filename.length] = '\0';
        printf("Sending file: %s -> %s\n", disk_path, _filename);
        extract_prg_from_image(_filename, &data_buffer);
        if (data_buffer.length == 0)
            return;
    }
    else if (disk_info.type == DISK_DIR) {
        char _disk_path[PATH_MAX + 1];
        strcpy(_disk_path, disk_path);

        int _disk_path_len = strlen(_disk_path);
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
            load_file();
        }
        else {
            FILE *fptr = fopen(_disk_path, "rb");
            if (fptr == NULL)
                return;
            data_buffer.length = dir_entry->filesize;
            fread(data_buffer.string, data_buffer.length, 1, fptr);
            printf("Sending file: %s (%ld bytes)\n", _disk_path, data_buffer.length);
            fclose(fptr);
        }
    }
}

void send_bytes() {
    printf("SENDING DATA\n");

    set_data(0);
    set_clock(1);

    load_file();
    if (data_buffer.length == 0) {
        fprintf(stderr, "%sERROR: can't send file %s\n", COLOR_RED, COLOR_RESET);
        microsleep(100);
        set_clock(0);
        //set_data(0);
        device_talking = filename.length = 0;
        return;
    }

    vic_byte c;

    int i_file = 0;
    int retry = 0;

    create_progress_bar();

    microsleep(80);

    do {
        if (resetted() || retry > 99) {
            device_resetted = 1;
            return;
        }
        if (atn(1)) {
            set_clock(0);
            printf("%sMACHINE ERROR%s\n", COLOR_RED, COLOR_RESET);
            return;
        }
        
        set_clock(0); // DEVICE IS READY TO SEND
        wait_data(0, 0); // COMPUTER IS READY FOR DATA

        int eoi = 0;
        
        c = data_buffer.string[i_file];
        i_file++;

        set_progress_bar(i_file * 100 / data_buffer.length);

        if (i_file == data_buffer.length) {
            eoi = 1;
            microsleep(200);
            wait_data(1, 0);
            wait_data(0, 0);
            microsleep(30);
        }

        #ifdef __linux__
            suseconds_t a;
            for (int i = 0; i < 8; i++) {
                a = get_microsec();
                set_clock(1);
                set_data((~c >> i) & 1);
                while ((get_microsec() - a) < 60) {}
                //microsleep(60 - (get_microsec() - a));

                a = get_microsec();
                set_clock(0);
                while ((get_microsec() - a) < 60) {}
                //microsleep(60 - (get_microsec() - a));
                set_data(0);
            }
        #elif _WIN64
            LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
            for (int i = 0; i < 8; i++) {
                QueryPerformanceCounter(&StartingTime);
                set_clock(1);
                set_data((~c >> i) & 1);
                QueryPerformanceCounter(&EndingTime);
                ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
                ElapsedMicroseconds.QuadPart *= 1000000;
                ElapsedMicroseconds.QuadPart /= lpFrequency.QuadPart;
                while (ElapsedMicroseconds.QuadPart < 60) {}

                QueryPerformanceCounter(&StartingTime);
                set_clock(0);
                QueryPerformanceCounter(&EndingTime);
                ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
                ElapsedMicroseconds.QuadPart *= 1000000;
                ElapsedMicroseconds.QuadPart /= lpFrequency.QuadPart;
                while (ElapsedMicroseconds.QuadPart < 60) {}
                set_data(0);
            }
        #endif

        set_clock(1);

        if (!wait_data(1, 1000)) {
            retry++;
            i_file--;
            //fseek(fptr, i_file, SEEK_SET);
            microsleep(100);
            continue;
        }
        retry = 0;

        microsleep(100); // BETWEEN BYTES TIME

        if (eoi) break;
    }
    while (1);

    set_clock(0);
    set_data(0);

    device_talking = 0;
}

void print_command_name(vic_byte command) {
    switch (command & 0xF0) {
        case LISTEN:
            printf("LISTEN ");
            break;
        case UNLISTEN:
            printf("UNLISTEN ");
            break;
        case TALK:
            printf("TALK ");
            break;
        case UNTALK:
            printf("UNTALK ");
            break;
        case OPEN_CHANNEL:
            printf("OPEN_CHANNEL ");
            break;
        case CLOSE:
            printf("CLOSE ");
            break;
        case OPEN:
            printf("OPEN ");
            break;
        default:
            printf("UNKNOWN COMMAND: %X\n", command);
            return;
    }

    vic_byte secondary = command & 0x0F;
    if (secondary != 0x0F) printf("%X", secondary);
    printf("\n");
}