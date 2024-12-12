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
    #if DEBUG
    printf("%s\n", __func__);
    #endif

    device_resetted = device_attentioned = device_listening = device_talking = 0;
}

void try_unfreeze() {
    set(CLOCK_HIGH); microsleep(100); set(CLOCK_LOW);
    //set(DATA_HIGH);  microsleep(1000); set(DATA_LOW);
}

void handle_atn() {
    #if DEBUG
    printf("\n%s\n", __func__);
    #endif

    char _localtime[LOCALTIME_STRLEN];
    get_localtime(_localtime);
    printf("[%s] %sATN ON%s -> ", _localtime, COLOR_CYAN, COLOR_RESET);
            
    set(CLOCK_LOW | DATA_HIGH);
    if (wait_clock(1, 1000000, 1)) goto error;

    do {
        if (wait_clock(0, 0, 1)) goto end;   // Talker is ready to send
        set(DATA_LOW);                       // Listener is ready for data

        if (wait_clock(1, 0, 1)) goto error;

        command = get_byte(1);
        print_command(command);

        if (command == LISTEN) {
            device_attentioned = device_listening = 1;
            device_talking = 0;
            continue;
        }
        else if((command == UNLISTEN) && device_listening) {
            device_attentioned = device_listening = 0;
        }
        else if (command == TALK) {
            device_attentioned = device_talking = 1;
            device_listening = 0;
            continue;
        }
        else if ((command == UNTALK) && device_talking) {
            device_attentioned = device_talking = 0;
        }
        else if (((command & 0xF0) == OPEN) && device_attentioned) {
            channel = command & 0x0F;
        }
        else if (((command & 0xF0) == CLOSE) && device_attentioned) {
            if ((command & 0x0F) == 1) get_disk_info();
            continue;
        }
        else if (((command & 0xF0) == SECOND) && device_attentioned) {
            channel = command & 0x0F;
        }
        else {
            error:
            try_unfreeze();
            set(CLOCK_LOW | DATA_LOW);
            device_attentioned = device_listening = device_talking = 0;
        }
        wait_atn(0);
        break;
    }
    while (atn(1) && !device_resetted);

    printf("%sATN OFF%s\n", COLOR_CYAN, COLOR_RESET);

    if (device_listening)
        receive_bytes();
    else if (device_talking)
        send_bytes();

    return;

    end:
    printf("%sATN OFF%s\n", COLOR_CYAN, COLOR_RESET);
}

void send_bytes() {
    #if DEBUG
    printf("sb ");
    #endif

    set(DATA_LOW | CLOCK_HIGH);

    write_data_buffer();

    vic_byte c;

    int i_file = 0;
    int error = 0;

    create_progress_bar();

    microsleep(80);

    do {
        #if DEBUG
        printf("\n%s%d%s ", COLOR_WHITE, (i_file + 1), COLOR_RESET);
        #endif
        
        if (resetted()) {
            device_resetted = 1;
            goto end;
        }
        else if (atn(1)) {
            set(CLOCK_LOW | DATA_HIGH);
            handle_atn();
            goto end;
        }
        else if (error) {
            char _localtime[LOCALTIME_STRLEN];
            get_localtime(_localtime);
            fprintf(stderr, "[%s] %sERROR: can't send file %s\n", _localtime, COLOR_RED, COLOR_RESET);
            device_attentioned = device_talking = filename.length = 0;
            set(CLOCK_LOW | DATA_LOW);
            goto end;
        }
        
        set(CLOCK_LOW);    // Device is ready to send
        wait_data(0, 0);   // Computer is ready for data

        if (data_buffer.length == 0) {   // Empty stream
            char _localtime[LOCALTIME_STRLEN];
            get_localtime(_localtime);
            fprintf(stderr, "[%s] %sERROR: file not found%s\n", _localtime, COLOR_RED, COLOR_RESET);
            device_attentioned = device_talking = filename.length = 0;
            set(CLOCK_LOW | DATA_LOW);
            goto end;
        }

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
            set(CLOCK_HIGH);
            a = get_microsec();
            set(((~c >> i) & 1) ? DATA_HIGH : DATA_LOW);
            while ((get_microsec() - a) < 70) {} // Bit setup

            set(CLOCK_LOW);
            a = get_microsec();
            while ((get_microsec() - a) < 20) {} // Data valid (20us for the VIC20, 60us for the Commodore 64)
            set(DATA_LOW);
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
            set(DATA_LOW);
        }
        #endif

        set(CLOCK_HIGH);

        if (!wait_data(1, 1000)) {
            i_file = data_buffer.length;
            continue;
        }

        microsleep(100);   // Between bytes time

        if (eoi) break;
    }
    while (1);

    set(CLOCK_LOW | DATA_LOW);

    end:
}

void receive_bytes() {
    #if DEBUG
    printf("%s\n", __func__);
    #endif

    data_buffer.length = 0;

    int _eoi = 0;
    do {
        wait_clock(0, 0, 0);   // Talker is ready to send
        set(DATA_LOW);      // Listener is ready for data
        
        if (eoi()) {
            set(DATA_HIGH);
            microsleep(60);
            set(DATA_LOW);
            _eoi = 1;
        }

        wait_clock(1, 0, 0);
        data_buffer.string[data_buffer.length++] = get_byte(0);

        microsleep(100);

        #if DEBUG
        printf("\n");
        #endif
    }
    while (!_eoi);

    handle_received_bytes();
}

void handle_received_bytes() {
    if (command == OPEN) {
        for (int i = 0; i < data_buffer.length; i++) {
            filename.string[i] = data_buffer.string[i];
            _filename_ascii[i] = p2a(data_buffer.string[i]);
        }
        filename.length = data_buffer.length;
        _filename_ascii[filename.length] = '\0';
        data_buffer.length = 0;

        char _localtime[LOCALTIME_STRLEN];
        get_localtime(_localtime);
        printf("[%s] %s%s:%s \"%s\"\n", _localtime, COLOR_YELLOW, (channel <= 1 ? "FILENAME" : "COMMAND"), COLOR_RESET, _filename_ascii);

        if (channel == 15)
            handle_dos_command();
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
                    goto exit_receive_bytes;
                }

                if (fwrite(data_buffer.string, data_buffer.length, 1, fptr) == 0) {
                    fprintf(stderr, "%sERROR: can't write file (errno %d)%s\n", COLOR_RED, errno, COLOR_RESET);
                    goto exit_receive_bytes;
                }

                char _localtime[LOCALTIME_STRLEN];
                get_localtime(_localtime);
                printf("[%s] %sSAVED:%s %s\n", _localtime, COLOR_YELLOW, COLOR_RESET, _filepath);

            exit_receive_bytes:
                fclose(fptr);
            }
            else if (disk_info.type == DISK_IMAGE) {
                char _localtime[LOCALTIME_STRLEN];
                get_localtime(_localtime);
                printf("[%s] %sSAVED:%s %s <- %s\n", _localtime, COLOR_YELLOW, COLOR_RESET, disk_path, _filename_ascii);
                save_file_to_image();
            }
        }
        else if (channel >= 2 && channel <= 14) {
            // TODO
        }
        else if (channel == 15)
            handle_dos_command();
    }
}

void handle_dos_command() {
    // TODO
}

void print_command(vic_byte command) {
    switch (command & 0xF0) {
        case (LISTEN & 0xF0):
            printf("LISTEN %d -> ", (command & 0x0F));
            break;
        case (UNLISTEN & 0xF0):
            printf("UNLISTEN -> ");
            break;
        case (TALK & 0xF0):
            printf("TALK %d -> ", (command & 0x0F));
            break;
        case (UNTALK & 0xF0):
            printf("UNTALK -> ");
            break;
        case (SECOND & 0xF0):
            printf("SECOND %d -> ", (command & 0x0F));
            break;
        case (CLOSE & 0xF0):
            printf("CLOSE %d -> ", (command & 0x0F));
            break;
        case (OPEN & 0xF0):
            printf("OPEN %d -> ", (command & 0x0F));
            break;
        default:
            printf("%sERROR%s UNKNOWN COMMAND: %X -> ", COLOR_RED, COLOR_RESET, command);
    }
}