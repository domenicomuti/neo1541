#include "device.h"

extern int addr;

vic_byte last_command;

int device_resetted = 1;
int device_attentioned = 0;
int device_listening = 0;
int device_talking = 0;
int open_mode;

#define MAX_DATA_BUFFER_SIZE 168656

vic_byte data_buffer[MAX_DATA_BUFFER_SIZE];
int i_data_buffer = 0;

vic_byte filename[100];
int i_filename = 0;

extern char* image;

#ifdef _WIN32
    extern LARGE_INTEGER lpFrequency;
#endif

int vic_string_equal(vic_byte* string1, vic_byte* string2, int n1, int n2) {
    if (n1 != n2) return 0;
    for (int i=0; i<n1; i++) {
        if (string1[n1] != string2[n1])
            return 0;
    }
    return 1;
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
        data_buffer[i_data_buffer] = get_byte();
        printf("%s%c - 0x%X\n%s", COLOR_YELLOW, data_buffer[i_data_buffer], data_buffer[i_data_buffer], COLOR_RESET);

        i_data_buffer++;

        set_data(1); // Frame Handshake

        microsleep(100);

        if (_eoi) break;
    }
    while (1);

    /*FILE *fptr = fopen("/home/noelyoung/list.prg", "a");
    fwrite(data_buffer, i_data_buffer, 1, fptr);
    fclose(fptr);*/

    if ((last_command & 0xF0) == OPEN) {
        for (int i=0; i<i_data_buffer; i++) {
            filename[i] = data_buffer[i];
        }
        i_filename = i_data_buffer;
        i_data_buffer = 0;
    }

    device_listening = 0;

    printf("%sGET DATA OFF\n%s", COLOR_MAGENTA, COLOR_RESET);
}

void read_directory() {
    struct vic_disk_info disk_info;
    get_disk_info(image, &disk_info);

    i_data_buffer = 0;
    for (int i=0; i<MAX_DATA_BUFFER_SIZE; i++) {
        data_buffer[i] = 0;
    }

    int i_memory_start = 0x1001;
    int i_memory = i_memory_start;
    int i_next_line;

    data_buffer[i_data_buffer++] = i_memory_start & 0x00FF; // Start
    data_buffer[i_data_buffer++] = (i_memory_start & 0xFF00) >> 8;
    i_next_line = i_data_buffer;

    data_buffer[4] = 0x00; // Line number 0
    data_buffer[5] = 0x00;

    i_data_buffer += 4;

    memcpy(data_buffer + i_data_buffer, disk_info.header, HEADER_SIZE);
    i_data_buffer += HEADER_SIZE;

    data_buffer[i_data_buffer] = 0x00; // new line
    i_data_buffer++;
    
    i_memory = i_memory_start + i_data_buffer - 2;
    data_buffer[i_next_line] = i_memory & 0x00FF;
    data_buffer[i_next_line + 1] = (i_memory & 0xFF00) >> 8;

    for (int i=0; i<disk_info.n_dir; i++) {
        i_next_line = i_data_buffer;
        
        i_data_buffer += 2;
        data_buffer[i_data_buffer++] = disk_info.dir[i].blocks & 0x00FF;
        data_buffer[i_data_buffer++] = (disk_info.dir[i].blocks & 0xFF00) >> 8;

        int start_offset;
        if (disk_info.dir[i].blocks < 10) start_offset = 3;
        else if (disk_info.dir[i].blocks < 100) start_offset = 2;
        else if (disk_info.dir[i].blocks < 1000) start_offset = 1;
        for (int j=0; j<start_offset; j++) {
            data_buffer[j + i_data_buffer] = ' ';
        }
        i_data_buffer += start_offset;

        memcpy(data_buffer + i_data_buffer, disk_info.dir[i].filename, FILENAMEMAXSIZE + 2);
        i_data_buffer += FILENAMEMAXSIZE + 2;

        memcpy(data_buffer + i_data_buffer, disk_info.dir[i].type, 5);
        i_data_buffer += 5;

        data_buffer[i_data_buffer] = 0x00; // new line
        i_data_buffer++;

        i_memory = i_memory_start + i_data_buffer;
        data_buffer[i_next_line] = i_memory & 0x00FF;
        data_buffer[i_next_line + 1] = (i_memory & 0xFF00) >> 8;
    }

    i_next_line = i_data_buffer;

    i_data_buffer += 2;
    data_buffer[i_data_buffer++] = disk_info.blocks_free & 0x00FF; // Start
    data_buffer[i_data_buffer++] = (disk_info.blocks_free & 0xFF00) >> 8;

    memcpy(data_buffer + i_data_buffer, (unsigned char*)"BLOCKS FREE.              ", 26);
    i_data_buffer += 26;
    data_buffer[i_data_buffer++] = 0;

    i_memory = i_memory_start + i_data_buffer;
    data_buffer[i_next_line] = i_memory & 0x00FF;
    data_buffer[i_next_line + 1] = (i_memory & 0xFF00) >> 8;

    data_buffer[i_data_buffer++] = 0;
    data_buffer[i_data_buffer++] = 0;

    FILE* fptr = fopen("image_examples/test_dir.prg", "wb");
    fwrite(data_buffer, i_data_buffer, 1, fptr);
    fclose(fptr);
}

void send_bytes() {
    printf("SENDING DATA\n");

    set_data(0);
    set_clock(1);

    /*FILE* fptr = fopen(image, "rb");
    fseek(fptr, 0, SEEK_END);
    int file_size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);*/
    
    if (vic_string_equal(filename, (vic_byte*)"$", i_filename, 1))
        read_directory();

    /*vic_byte prg_buffer[16384];
    int file_size = 0;
    extract_prg_from_image("/home/noelyoung/omega.d64", "omega race", prg_buffer, &file_size);*/

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
        
        //c = getc(fptr);
        c = data_buffer[i_file];
        i_file++;

        set_progress_bar(i_file * 100 / i_data_buffer);

        if (i_file == i_data_buffer) {
            eoi = 1;
            microsleep(200);
            wait_data(1, 0);
            wait_data(0, 0);
            microsleep(30);
        }

        #ifdef __linux__
            suseconds_t a;
            for (int i=0; i<8; i++) {
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
        #elif _WIN32
            LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
            for (int i=0; i<8; i++) {
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

    //fclose(fptr);

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