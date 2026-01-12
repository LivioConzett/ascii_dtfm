/**
 * ASCII to DTMF encoder
 * 
 * gcc -std=c99 -Werror -Wall main.c -lm
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846


// Create Wav header Struct
// https://docs.fileformat.com/audio/wav/
struct wav_header
{
    char riff[4];             /* "RIFF"                                        */
    int32_t file_length;      /* file length in bytes                          */
    char wave[4];             /* "WAVE"                                        */
    char fmt[4];              /* "fmt "                                        */
    int32_t chunk_size;       /* size of FMT chunk in bytes (usually 16)       */
    int16_t format_tag;       /* 1=PCM, 257=Mu-Law, 258=A-Law, 259=ADPCM       */
    int16_t num_of_chanels;   /* 1=mono, 2=stereo                              */
    int32_t sample_rate;      /* Sampling rate in samples per second           */
    int32_t bytes_per_sec;    /* bytes per second = sample_rate*bytes_per_samp */
    int16_t bytes_per_sample; /* 2=16-bit mono, 4=16-bit stereo                */
    int16_t bits_per_sample;  /* Number of bits per sample                     */
    char data[4];             /* "data"                                        */
    int32_t data_length;      /* data length in bytes (filelength - 44)        */
};


const int sample_rate = 8000;

// the DTMF frequency pairs
// https://en.wikipedia.org/wiki/DTMF_signaling
const uint16_t frequencies[16][2] = {
    {1336, 941}, // 0
    {1209, 697}, // 1
    {1336, 697}, // 2
    {1477, 697}, // 3
    {1209, 770}, // 4
    {1336, 770}, // 5
    {1477, 770}, // 6
    {1209, 852}, // 7
    {1336, 852}, // 8
    {1477, 852}, // 9
    {1633, 697}, // A
    {1633, 770}, // B
    {1633, 852}, // C
    {1633, 941}, // D
    {1209, 941}, // * in our case E
    {1477, 941}  // # in our case F
};

/**
 * \brief print the wav header
 * \param header header to print
 */
void print_header(struct wav_header header){

    printf("            riff: %c%c%c%c\n",header.riff[0], header.riff[1], header.riff[2], header.riff[3]);
    printf("     file length: %d\n",header.file_length);
    printf("            wave: %c%c%c%c\n",header.wave[0], header.wave[1], header.wave[2], header.wave[3]);
    printf("             fmt: %c%c%c%c\n",header.fmt[0], header.fmt[1], header.fmt[2], header.fmt[3]);
    printf("      chunk size: %d\n",header.chunk_size);
    printf("      format tag: %d\n",header.format_tag);
    printf(" num of channels: %d\n",header.num_of_chanels);
    printf("     sample rate: %d\n",header.sample_rate);
    printf("   bytes per sec: %d\n",header.bytes_per_sec);
    printf("bytes per sample: %d\n",header.bytes_per_sample);
    printf(" bits per sample: %d\n",header.bits_per_sample);
    printf("            data: %c%c%c%c\n",header.data[0], header.data[1], header.data[2], header.data[3]);
    printf("     data length: %d\n",header.data_length);
    
}


uint16_t calculate_dtmf(uint8_t number, uint16_t index, uint16_t sample_rate){

    return (uint16_t) ((cos((2 * PI * frequencies[number][0] * index) / sample_rate) + 
                        cos((2 * PI * frequencies[number][1] * index) / sample_rate)) * 10000);
    // return (uint16_t)(sin((2 * PI * 440 * index) / sample_rate) * 10000);
}


/**
 * Main entry point for the program
 */
int main(int argc, char* argv[]){

    double length_of_note_s = 0.1f;
    double length_of_pause_s = 0.05f;

    // make sure arguments are given
    if(argc < 2){
        printf("Not enough arguments given!\n");
        exit(EXIT_FAILURE);
    }


    // index for the character in the argv
    uint32_t char_index = 0;

    // get the length of the string

    // go through the string
    while(argv[1][char_index] != 0){
        char_index++;
    }
    printf("amount of chars: %d\n", char_index);

    // two nibbles per character
    long amount_of_chars = char_index;
    long amount_of_nibbles = amount_of_chars * 2;

    struct wav_header w_header;

    strncpy(w_header.riff, "RIFF", 5);
    strncpy(w_header.wave, "WAVE", 5);
    strncpy(w_header.fmt, "fmt ", 5);
    strncpy(w_header.data, "data", 5);

    w_header.chunk_size = 16;
    w_header.format_tag = 1;
    w_header.num_of_chanels = 1;
    w_header.sample_rate = sample_rate;
    w_header.bits_per_sample = 16;
    w_header.bytes_per_sec = (w_header.sample_rate * w_header.bits_per_sample / 8) * w_header.num_of_chanels;
    w_header.bytes_per_sample = w_header.bits_per_sample / 8 * w_header.num_of_chanels;

    double duration_in_sec = ((amount_of_nibbles * length_of_note_s) + (amount_of_nibbles * length_of_pause_s));
    printf("duration in seconds: %f\n", duration_in_sec);
    uint16_t buffer_size = sample_rate * duration_in_sec;

    uint16_t buffer[buffer_size];
    uint16_t buffer_index = 0;

    for(int char_counter = 0; char_counter < amount_of_chars; char_counter++){
        
        // split the byte into the most significant and least significant nibble
        uint8_t ms_nibble = (uint8_t) (argv[1][char_counter] & 0b11110000) >> 4;
        uint8_t ls_nibble = (uint8_t) argv[1][char_counter] & 0b00001111;

        printf("%d",ms_nibble);

        // note sound
        for(int i = 0; i < (sample_rate * length_of_note_s); i++){
            buffer[buffer_index] = calculate_dtmf(ms_nibble, i, sample_rate);
            buffer_index++;
        }

        // pause
        for(int i = 0; i < (sample_rate * length_of_pause_s); i++){
            buffer[buffer_index] = 0;
            buffer_index++;
        }

        printf("%d",ls_nibble);

        // note sound
        for(int i = 0; i < (sample_rate * length_of_note_s); i++){
            buffer[buffer_index] = calculate_dtmf(ls_nibble, i, sample_rate);
            buffer_index++;
        }

        // pause
        for(int i = 0; i < (sample_rate * length_of_pause_s); i++){
            buffer[buffer_index] = 0;
            buffer_index++;
        }

    }

    printf("\n");


    w_header.data_length = buffer_size * w_header.bytes_per_sample;
    w_header.file_length = w_header.data_length + sizeof(struct wav_header);

    print_header(w_header);

    // Writing Wav File to Disk
    FILE *fp = fopen("test.wav", "w");
    fwrite(&w_header, 1, sizeof(struct wav_header), fp);
    fwrite(buffer, 2, buffer_size, fp);

    return 0;
}