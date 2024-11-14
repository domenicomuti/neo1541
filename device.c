#include "device.h"

extern int addr;

vic_byte command;
vic_byte channel;

int device_resetted = 1;
int device_attentioned = 0;
int device_listening = 0;
int device_talking = 0;

extern char disk_path[PATH_MAX];
extern vic_disk_info disk_info;

vic_string data_buffer;
vic_string filename;
char _filename_ascii[NAME_MAX + 1];

#ifdef _WIN64
extern LARGE_INTEGER lpFrequency;
#endif

void initialize_buffers() {
#if DEBUG
    printf("%s\n", __func__);
#endif
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
#if DEBUG
    printf("%s\n", __func__);
#endif
    free(data_buffer.string);
    free(filename.string);
}

void reset_device() {
#if DEBUG
    printf("%s\n", __func__);
#endif
    device_resetted = device_attentioned = device_listening = device_talking = 0;
}

vic_byte get_byte() {
#if DEBUG
    printf("%s\n", __func__);
#endif
    vic_byte byte = 0;

    // bit 1
    wait_clock(0, 0); // 70 timeout
    byte = (INB(addr + 1) & 0x40) >> 6;
    wait_clock(1, 0); // 20 timeput

    // bit 2
    wait_clock(0, 0); // 70 timeout
    byte |= (INB(addr + 1) & 0x40) >> 5;
    wait_clock(1, 0); // 20 timeput

    // bit 3
    wait_clock(0, 0); // 70 timeout
    byte |= (INB(addr + 1) & 0x40) >> 4;
    wait_clock(1, 0); // 20 timeput

    // bit 4
    wait_clock(0, 0); // 70 timeout
    byte |= (INB(addr + 1) & 0x40) >> 3;
    wait_clock(1, 0); // 20 timeput

    // bit 5
    wait_clock(0, 0); // 70 timeout
    byte |= (INB(addr + 1) & 0x40) >> 2;
    wait_clock(1, 0); // 20 timeput
    
    // bit 6
    wait_clock(0, 0); // 70 timeout
    byte |= (INB(addr + 1) & 0x40) >> 1;
    wait_clock(1, 0); // 20 timeput

    // bit 7
    wait_clock(0, 0); // 70 timeout
    byte |= (INB(addr + 1) & 0x40);
    wait_clock(1, 0); // 20 timeput

    // bit 8
    wait_clock(0, 0); // 70 timeout
    byte |= (INB(addr + 1) & 0x40) << 1;
    wait_clock(1, 0); // 20 timeput

    return byte;
}

void handle_atn() {
#if DEBUG
    printf("%s\n", __func__);
#endif
    printf("[%ld] %sATN ON%s -> ", get_microsec(), COLOR_CYAN, COLOR_RESET);
            
    set_data(1);
    wait_clock(1, 1000); // TODO CHECK TIMEOUT

    do {
        wait_clock(0, 0); // Talker is ready to send
        set_data(0);   // Listener is ready for data

        wait_clock(1, 0);
        command = get_byte();

        if (valid_command(command)) {
            if ((command & 0xF0) == LISTEN) {
                device_attentioned = device_listening = ((command & 0x0F) == DEVICE);
                device_talking = 0;
            }
            else if((command & 0xF0) == UNLISTEN) {
                device_listening = 0;
            }
            else if ((command & 0xF0) == OPEN) {
                channel = command & 0x0F;
            }
            else if ((command & 0xF0) == TALK) {
                device_listening = 0;
                device_attentioned = device_talking = ((command & 0x0F) == DEVICE);
            }
            else if ((command & 0xF0) == UNTALK) {
                device_talking = 0;
            }
            else if ((command & 0xF0) == SECOND) {
                channel = command & 0x0F;
            }
            else if ((command & 0xF0) == CLOSE) {
                if ((command & 0x0F) == 1) get_disk_info();
            }
            else {
                device_attentioned = device_listening = device_talking = 0;
            }

            if (!device_attentioned) {
                wait_atn(0);
                continue;
            }

            set_data(1); // Frame Handshake
            wait_clock(0, 0);

            if (((command & 0xF0) == UNLISTEN) || ((command & 0xF0) == UNTALK))
                device_attentioned = 0;
        }
        else {
            device_attentioned = device_listening = device_talking = 0;
            //set_data(1);    
            wait_clock(0, 0);
        }
    }
    while (atn(1) && !device_resetted);

    printf("%sATN OFF%s\n", COLOR_CYAN, COLOR_RESET);
}

void receive_bytes() {
#if DEBUG
    printf("%s\n", __func__);
#endif
    data_buffer.length = 0;

    int _eoi = 0;
    do {
        wait_clock(0, 0); // Talker is ready to send
        set_data(0);   // Listener is ready for data
        
        if (eoi()) {
            set_data(1);
            microsleep(60);
            set_data(0);
            _eoi = 1;
        }

        wait_clock(1, 0);
        data_buffer.string[data_buffer.length++] = get_byte();

        set_data(1); // Frame Handshake
        microsleep(100);
    }
    while (!_eoi);
    
    if ((command & 0xF0) == OPEN) {
        for (int i = 0; i < data_buffer.length; i++) {
            filename.string[i] = data_buffer.string[i];
            _filename_ascii[i] = p2a(data_buffer.string[i]);
        }
        filename.length = data_buffer.length;
        _filename_ascii[filename.length] = '\0';
        data_buffer.length = 0;

        printf("[%ld] %s%s:%s \"%s\"\n", get_microsec(), COLOR_YELLOW, (channel <= 1 ? "FILENAME" : "COMMAND"), COLOR_RESET, _filename_ascii);

        /*  I added the goto statement to manage a strange behaviour from the VIC20 (don't know if is the same for the C64)
            
            OPEN 15,8,15,"UI-":CLOSE 15   <--- with this command the machine sends OPEN
            
            OPEN 15,8,15
            PRINT# 15,"UI-"   <--- with this command the machine sends SECOND
            CLOSE 15
        */
        if (channel == 15)
            goto dos_command;
    }
    else if ((command & 0xF0) == SECOND) {
        if (channel == 1) {
            // Save
            if (disk_info.type == DISK_DIR) {
                char _filepath[PATH_MAX];
                int _filepath_len = strlen(disk_path);
                memcpy(_filepath, disk_path, _filepath_len);
                _filepath[_filepath_len] = FILESEPARATOR;
                strcpy(_filepath + _filepath_len + 1, _filename_ascii);

                FILE* fptr = fopen(_filepath, "wb");
                if (fptr == NULL) {
                    fprintf(stderr, "%sERROR: can't open file %s (errno %d)%s\n", COLOR_RED, _filepath, errno, COLOR_RESET);
                    // TODO: gestire il caso in cui si va a scrivere una directory (errno 21)
                }

                if (fwrite(data_buffer.string, data_buffer.length, 1, fptr) == 0)
                    fprintf(stderr, "%sERROR: can't write file (errno %d)%s\n", COLOR_RED, errno, COLOR_RESET);

                printf("%sSAVING FILE:%s %s\n", COLOR_YELLOW, COLOR_RESET, _filepath);
                fclose(fptr);
            }
            else if (disk_info.type == DISK_IMAGE) {
                printf("%sSAVING FILE:%s %s <- %s\n", COLOR_YELLOW, COLOR_RESET, disk_path, _filename_ascii);
                save_file_to_image();
            }
        }
        else if (channel >= 2 && channel <= 14) {
            // TODO
        }
        else if (channel == 15) {
            dos_command:
            // TODO
        }
    }

    device_listening = 0;
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

void load_file() {
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
        load_file();
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
            load_file();
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

void send_bytes() {
#if DEBUG
    printf("%s\n", __func__);
#endif
    set_data(0);
    set_clock(1);

    load_file();
    if (data_buffer.length == 0) {
        fprintf(stderr, "[%ld] %sERROR: can't send file %s\n", get_microsec(), COLOR_RED, COLOR_RESET);
        microsleep(100);
        set_clock(0);
        device_talking = filename.length = 0;
        return;
    }

    vic_byte c;

    int i_file = 0;
    int retry = 0;

    create_progress_bar();

    microsleep(80);

    do {
        if (resetted() || atn(1) || retry > 99) {
            device_resetted = 1;
            return;
        }
        
        set_clock(0);       // Device is ready to send
        wait_data(0, 0);    // Computer is ready for data

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
            set_clock(1);
            a = get_microsec();
            set_data((~c >> i) & 1);
            while ((get_microsec() - a) < 70) {} // Bit setup

            set_clock(0);
            a = get_microsec();
            while ((get_microsec() - a) < 20) {} // Data valid (20 for the VIC20, 60 for the Commodore 64)
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
            microsleep(100);
            continue;
        }
        retry = 0;

        microsleep(100); // Between bytes time

        if (eoi) break;
    }
    while (1);

    set_clock(0);
    set_data(0);

    device_talking = 0;
}

int valid_command(vic_byte command) {
    switch (command & 0xF0) {
        case LISTEN:
            printf("LISTEN %d -> ", command & 0x0F);
            break;
        case UNLISTEN:
            printf("UNLISTEN -> ");
            break;
        case TALK:
            printf("TALK %d -> ", command & 0x0F);
            break;
        case UNTALK:
            printf("UNTALK -> ");
            break;
        case SECOND:
            printf("SECOND %d -> ", command & 0x0F);
            break;
        case CLOSE:
            printf("CLOSE %d -> ", command & 0x0F);
            break;
        case OPEN:
            printf("OPEN %d -> ", command & 0x0F);
            break;
        default:
            printf("%sERROR%s UNKNOWN COMMAND: %X -> ", COLOR_RED, COLOR_RESET, command);
            return 0;
    }
    return 1;
}