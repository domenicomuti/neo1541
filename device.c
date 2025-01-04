#include "device.h"

extern int port;
extern int vic20_mode;

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

extern WINDOW *header_window;
extern WINDOW *log_window;

int header_i = 0;

#ifdef _WIN64
extern LARGE_INTEGER lpFrequency;
#endif

void init_buffers() {
    vic_byte *_data_buffer = malloc(MAX_DATA_BUFFER_SIZE * sizeof(vic_byte));
    if (_data_buffer == NULL) {
        destroy_gui();
        printf("ERROR Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    data_buffer.string = _data_buffer;
    data_buffer.length = 0;

    vic_byte *_filename = malloc(FILENAMEMAXSIZE * sizeof(vic_byte));
    if (_filename == NULL) {
        destroy_gui();
        printf("ERROR Memory allocation error\n");
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

void try_unfreeze() {
    int sleep = 100000;
    for (int i = 0; i < 2; i++) {
        microsleep(sleep);
        set(CLOCK_HIGH);
        microsleep(sleep);
        set(CLOCK_LOW);

        microsleep(sleep);
        set(DATA_HIGH); 
        microsleep(sleep);
        set(DATA_LOW);
    }
}

void handle_atn() {
    #if DEBUG
    printf("\n%s\n", __func__);
    #endif

    print_log("ATN ON", 1, 5, 0);
    print_log(" -> ", 0, 0, 0);
            
    set(CLOCK_LOW | DATA_HIGH);
    if (wait_clock(1, 200, 1)) goto error;

    do {
        if (wait_clock(0, 5000, 1)) goto end;   // Talker is ready to send
        set(DATA_LOW);                          // Listener is ready for data

        if (wait_clock(1, 100, 1)) goto error;

        vic_byte new_command = get_byte(1);
        if (command == TALK && ((new_command & 0xF0) != SECOND)) {
            command = new_command;
            goto error;
        }
        command = new_command;

        print_command();

        if (command == LISTEN) {
            device_attentioned = device_listening = 1;
            device_talking = 0;
            continue;
        }
        else if (command == UNLISTEN) {
            device_attentioned = device_listening = 0;
        }
        else if (command == TALK) {
            device_attentioned = device_talking = 1;
            device_listening = 0;
            continue;
        }
        else if (command == UNTALK) {
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
            set(CLOCK_LOW | DATA_LOW);
            device_attentioned = device_listening = device_talking = 0;
            try_unfreeze();
        }
        wait_atn(0);
        break;
    }
    while (atn(1) && !device_resetted);

    print_log("ATN OFF\n", 0, 5, 1);

    if (device_listening)
        download_bytes();
    else if (device_talking)
        upload_bytes();

    return;

    end:
    print_log("ATN OFF\n", 0, 5, 1);
}

void download_bytes() {
    #if DEBUG
    printf("\n%s\n", __func__);
    #endif

    data_buffer.length = 0;

    int _eoi = 0;
    int header_j = 0;
    do {
        header_i %= 7;
        header_j %= 64;
        if (header_j == 0) {
            print_header(header_i++, 0);
            wrefresh(header_window);
        }
        header_j++;

        #if DEBUG
        printf("=>");
        #endif
        if (wait_clock(0, 2000, 0)) goto end;   // Talker is ready to send
        set(DATA_LOW);                          // Listener is ready for data
        
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

    end:
}

void upload_bytes() {
    #if DEBUG
    printf("\n%s\n", __func__);
    #endif

    set(DATA_LOW | CLOCK_HIGH);

    write_data_buffer();

    if (data_buffer.length == 0) {   // Empty stream
        char _localtime[LOCALTIME_STRLEN];
        get_localtime(_localtime);
        print_log("ERROR", 1, 1, 0);
        print_log(" file not found\n", 0, 0, 1);
        device_attentioned = device_talking = filename.length = 0;
        goto end;
    }

    vic_byte c;

    int i_file = 0;
    int error = 0;

    int data_valid_window = vic20_mode ? 20 : 60;

    microsleep(80);

    int header_j = 0;
    do {
        header_i %= 7;
        header_j %= 64;
        if (header_j == 0) {
            print_header(header_i++, 1);
            wrefresh(header_window);
        }
        header_j++;

        #if DEBUG
        {
            char _localtime[LOCALTIME_STRLEN];
            get_localtime(_localtime);
            print_log("\n[%s] %d ", 0, 0, 1, _localtime, (i_file + 1));
        }
        #endif
        
        if (resetted()) {
            device_resetted = 1;
            goto end;
        }
        else if (atn(1)) {
            goto end;
        }
        else if (error) {
            print_log("ERROR", 1, 1, 0);
            print_log(" can't send file\n", 0, 0, 1);
            device_attentioned = device_talking = filename.length = 0;
            goto end;
        }
        
        set(CLOCK_LOW);    // Device is ready to send
        wait_data(0, 0);   // Computer is ready for data

        int _eoi = 0;
        
        c = data_buffer.string[i_file];
        i_file++;

        set_progress_bar(i_file * 100 / data_buffer.length);

        if (i_file >= data_buffer.length) {
            _eoi = 1;
            microsleep(255);
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
            while ((get_microsec() - a) < 70) {}                  // Bit setup

            set(CLOCK_LOW);
            a = get_microsec();
            while ((get_microsec() - a) < data_valid_window) {}   // Data valid
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

        if (wait_data(1, 1000)) goto error;

        microsleep(100);   // Between bytes time

        if (_eoi) break;
    }
    while (1);

    end:
    #if DEBUG
    printf("END ");
    #endif
    set(CLOCK_LOW | DATA_LOW);
    return;

    error:
    #if DEBUG
    printf("ERR ");
    #endif
    set(CLOCK_LOW | DATA_LOW);
    try_unfreeze();
}

void handle_received_bytes() {
    if ((command & 0xF0) == OPEN) {
        for (int i = 0; i < data_buffer.length; i++) {
            filename.string[i] = data_buffer.string[i];
            _filename_ascii[i] = p2a(data_buffer.string[i]);
        }
        filename.length = data_buffer.length;
        _filename_ascii[filename.length] = '\0';
        data_buffer.length = 0;

        print_log(channel <= 1 ? "FILENAME" : "COMMAND", 1, 4, 0);
        print_log(" \"%s\"\n", 0, 0, 1, _filename_ascii);

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
                    print_log("ERROR", 1, 1, 0);
                    print_log(" can't open file %s (errno %d)\n", 0, 0, 1, _filepath, errno);
                    // TODO: gestire il caso in cui si va a scrivere una directory (errno 21)
                    goto exit_download_bytes;
                }

                if (fwrite(data_buffer.string, data_buffer.length, 1, fptr) == 0) {
                    print_log("ERROR", 1, 1, 0);
                    print_log(" can't write file (errno %d)\n", 0, 0, 1, errno);
                    goto exit_download_bytes;
                }

                print_log("SAVED ", 1, 4, 0);
                print_log("%s (%d bytes)\n", 0, 0, 0, _filepath, data_buffer.length);
                
                exit_download_bytes:
                fclose(fptr);
            }
            else if (disk_info.type == DISK_IMAGE) {
                print_log("SAVED ", 1, 4, 0);
                print_log("%s <- %s (%d bytes)\n", 0, 0, 0, disk_path, _filename_ascii, data_buffer.length);
                save_file_to_image();
            }
        }
        else if (channel >= 2 && channel <= 14) {
            // TODO
        }
        else if (channel == 15) {
            for (int i = 0; i < data_buffer.length; i++) {
                filename.string[i] = data_buffer.string[i];
                _filename_ascii[i] = data_buffer.string[i];
            }
            filename.length = data_buffer.length;
            _filename_ascii[filename.length] = '\0';
            data_buffer.length = 0;

            print_log("DOS COMMAND", 1, 4, 0);
            print_log(" \"%s\"\n", 0, 0, 1, _filename_ascii);

            handle_dos_command();
        }
    }
}

void handle_dos_command() {
    // TODO
}

void print_command() {
    switch (command & 0xF0) {
        case (LISTEN & 0xF0):
            print_log("LISTEN %d -> ", 0, 0, 1, (command & 0x0F));
            break;
        case (UNLISTEN & 0xF0):
            print_log("UNLISTEN -> ", 0, 0, 1);
            break;
        case (TALK & 0xF0):
            print_log("TALK %d -> ", 0, 0, 1, (command & 0x0F));
            break;
        case (UNTALK & 0xF0):
            print_log("UNTALK -> ", 0, 0, 1);
            break;
        case (SECOND & 0xF0):
            print_log("SECOND %d -> ", 0, 0, 1, (command & 0x0F));
            break;
        case (CLOSE & 0xF0):
            print_log("CLOSE %d -> ", 0, 0, 1, (command & 0x0F));
            break;
        case (OPEN & 0xF0):
            print_log("OPEN %d -> ", 0, 0, 1, (command & 0x0F));
            break;
        default:
            print_log("ERROR UNKNOWN COMMAND: %X -> ", 0, 0, 1, command);
    }
}