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
    char riff[4];             /* "RIFF"                                  */
    int32_t file_length;      /* file length in bytes                    */
    char wave[4];             /* "WAVE"                                  */
    char fmt[4];              /* "fmt "                                  */
    int32_t chunk_size;       /* size of FMT chunk in bytes (usually 16) */
    int16_t format_tag;       /* 1=PCM, 257=Mu-Law, 258=A-Law, 259=ADPCM */
    int16_t num_of_chanels;   /* 1=mono, 2=stereo                        */
    int32_t sample_rate;      /* Sampling rate in samples per second     */
    int32_t bytes_per_sec;    /* bytes per second = srate*bytes_per_samp */
    int16_t bytes_per_sample; /* 2=16-bit mono, 4=16-bit stereo          */
    int16_t bits_per_sample;  /* Number of bits per sample               */
    char data[4];             /* "data"                                  */
    int32_t data_length;      /* data length in bytes (filelength - 44)  */
};


const int sample_rate = 8000;



/**
 * Main entry point for the program
 */
int main(int argc, char* argv[]){

    // make sure arguments are given
    if(argc < 2){
        printf("Not enough arguments given!\n");
        exit(EXIT_FAILURE);
    }


    // index for the character in the argv
    uint32_t char_index = 0;

    // go through the string
    while(argv[1][char_index] != 0){
        
        // split the byte into the most significant and least significant nibble
        uint8_t ms_nibble = (uint8_t) (argv[1][char_index] & 0b11110000) >> 4;
        uint8_t ls_nibble = (uint8_t) argv[1][char_index] & 0b00001111;

        printf("%x%x ", ms_nibble, ls_nibble);
        char_index++;
    }
    printf("\n");

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

    uint8_t duration_in_sec = 1;
    uint16_t buffer_size = sample_rate * duration_in_sec;

    uint16_t buffer[buffer_size];

    for(int i = 0; i < buffer_size; i++){
        buffer[i] = (uint16_t)((cos((2 * PI * 440 * i) / sample_rate) * 10000));
        // printf("%d\n", buffer[i]);
    }

    w_header.data_length = buffer_size * w_header.bytes_per_sample;
    w_header.file_length = w_header.data_length + sizeof(struct wav_header);

    // Writing Wav File to Disk
    FILE *fp = fopen("test.wav", "w");
    fwrite(&w_header, 1, sizeof(struct wav_header), fp);
    fwrite(buffer, 2, buffer_size, fp);

    return 0;
}