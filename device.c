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

void send_bytes() {
#if DEBUG
    printf("%s\n", __func__);
#endif
    set_data(0);
    set_clock(1);

    write_data_buffer();
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