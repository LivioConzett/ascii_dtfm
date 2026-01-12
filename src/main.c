/**
 * ASCII to DTMF encoder
 * 
 * gcc -std=c99 -Werror -Wall main.c -lm
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define PI 3.14159265358979323846

// Create Wav header Struct
// https://docs.fileformat.com/audio/wav/
struct wav_header {
    char riff[4];             // "RIFF"                                        
    int32_t file_length;      // file length in bytes                          
    char wave[4];             // "WAVE"                                        
    char fmt[4];              // "fmt "                                        
    int32_t chunk_size;       // size of FMT chunk in bytes (usually 16)       
    int16_t format_tag;       // 1=PCM, 257=Mu-Law, 258=A-Law, 259=ADPCM       
    int16_t num_of_chanels;   // 1=mono, 2=stereo                              
    int32_t sample_rate;      // Sampling rate in samples per second           
    int32_t bytes_per_sec;    // bytes per second = sample_rate*bytes_per_samp 
    int16_t bytes_per_sample; // 2=16-bit mono, 4=16-bit stereo                
    int16_t bits_per_sample;  // Number of bits per sample                     
    char data[4];             // "data"                                        
    int32_t data_length;      // data length in bytes (filelength - 44)        
};


// params for the program
struct params {
    double note_length_s;     // length of the note in seconds
    double pause_length_s;    // length of the pause in seconds
    uint16_t sample_rate;     // sample rate to sample the audio
    char* file_output;        // file to output the data to
    char* data;               // pointer to the input string
};


// states for the parameter parsing
enum param_state {
    NORMAL,
    NOTE_LENGTH,
    PAUSE_LENGTH,
    FILE_OUTPUT
};


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

const char default_file_output[] = "dtmf_output.wav";
const double default_note_length = 0.3f;
const double default_pause_length = 0.1f;

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

/**
 * \brief calculates the dtmf signal for a hex number at a certain point
 * \param number number to convert
 * \param index point in the signal to calculate for
 * \param sample_rate the sample rate
 * \return the signal at that point for a number
 */
uint16_t calculate_dtmf(uint8_t number, uint16_t index, uint16_t sample_rate){

    return (uint16_t) ((cos((2 * PI * frequencies[number][0] * index) / sample_rate) + 
                         cos((2 * PI * frequencies[number][1] * index) / sample_rate)) * 10000);
}


/**
 * \brief copy a string to a destination
 * \param destination where to copy the string to
 * \param string string to copy
 * \param amount_of_bytes amount of bytes to copy
 */
void copy_string(char destination[], char string[], uint16_t amount_of_bytes){
    for(int i = 0; i < amount_of_bytes; i++){
        if(string[i] == 0){
            printf("\nERROR Copying String '%s'!\nString is shorter than %d bytes!\n\n", string, amount_of_bytes);
            exit(EXIT_FAILURE);
        }
        destination[i] = string[i];
    }
}

/**
 * \brief prints the help
 */
void print_help(){
    printf("\n");
    printf("This program converts a string of ascii characters into a wav file of dtmf beeps.\n\n");
    printf("Usage eg:\n");
    printf("  dtmf_ascii \"hello world\" -p 500 -n 100 -o \"hello.wav\"\n");
    printf("  dtmf_ascii \"yo dude\"\n");
    printf("\n");
    printf("parameter       meaning\n");
    printf("----------------------------------------\n");
    printf(" -h              shows this help screen\n");
    printf(" -n <length>     sets how long a beep is active [ms]\n");
    printf("                 default: %dms\n", (uint16_t) (default_note_length * 1000));
    printf(" -o <filename>   sets the output file\n");
    printf("                 default: '%s'\n", default_file_output);
    printf(" -p <length>     sets how long the pause between beeps is [ms]\n");
    printf("                 default: %dms\n", (uint16_t) (default_pause_length * 1000));
    printf("\n");
}


/**
 * \brief Creates the params from the argv given
 * \param param the parmas struct to write to
 * \param argc the amount of arguments given
 * \param argv the place where the arguments are
 */
void parse_parameters(struct params* param, int argc, char* argv[]){

    if(argc < 2){
        printf("Not enough arguments given!\n");
        exit(EXIT_FAILURE);
    }

    enum param_state state = NORMAL;

    // set the defaults
    param->note_length_s = default_note_length;
    param->pause_length_s = default_pause_length;
    param->sample_rate = 8000;
    param->data = 0;
    param->file_output = (char*) &default_file_output;

    // parse the arguments given
    // start at 1 since we don't want the name of the program 
    for(int i = 1; i < argc; i++){
        
        switch(state){
            // normal mode
            case NORMAL:
                if(argv[i][0] == '-'){

                    switch(argv[i][1]){
                        case 'n':
                            state = NOTE_LENGTH;
                            break;
                        case 'p':
                            state = PAUSE_LENGTH;
                            break;
                        case 'o':
                            state = FILE_OUTPUT;
                            break;
                        case 'h':
                            print_help();
                            exit(EXIT_SUCCESS);
                    }
                } 
                else {
                    param->data = argv[i];
                }
                break;
            // note length mode
            case NOTE_LENGTH:
                param->note_length_s = atoi(argv[i]) / 1000.0;
                state = NORMAL;
                break;
            // pause length mode
            case PAUSE_LENGTH:
                param->pause_length_s = atoi(argv[i]) / 1000.0;
                state = NORMAL;
                break;
            // set the file output
            case FILE_OUTPUT:
                param->file_output = argv[i];
                state = NORMAL;
                break;
        }
    }
}

/**
 * \brief Checks if all the values in the params are within speck
 * \param param the parameters
 */
void validate_parameters(struct params* param){

    if(param->data == 0){
        printf("\nERROR no data to encode given!\n\n");
        exit(EXIT_FAILURE);
    }

    if(param->note_length_s == 0){
        printf("WARNING: the note length is 0.0s!\n");
    }

    if(param->pause_length_s == 0){
        printf("WARNING: the pause length is 0.0s!\n");
    }

    if(param->file_output == 0){
        printf("\nERROR: No output file given!\n\n");
        exit(EXIT_FAILURE);
    }

}


/**
 * \brief Main entry point for the program
 */
int main(int argc, char* argv[]){

    // create the parameters use in the program
    struct params parameters;

    // parse the params from the arg
    parse_parameters(&parameters, argc, argv);

    validate_parameters(&parameters);

    printf("    note length: %fs\n", parameters.note_length_s);
    printf("   pause length: %fs\n", parameters.pause_length_s);
    printf(" data to encode: %s\n", parameters.data);

    // index for the character in the argv
    uint32_t char_index = 0;

    // get the length of the string
    while(parameters.data[char_index] != 0){
        char_index++;
    }

    uint16_t amount_of_chars = char_index;
    // two nibbles per character
    uint16_t amount_of_nibbles = amount_of_chars * 2;

    printf("amount of chars: %d\n", amount_of_chars);
    printf("amount of beeps: %d\n", amount_of_nibbles);
    printf("    output file: %s\n", parameters.file_output);

    // create the header
    struct wav_header w_header;

    copy_string(w_header.riff, "RIFF", 4);
    copy_string(w_header.wave, "WAVE", 4);
    copy_string(w_header.fmt, "fmt ", 4);
    copy_string(w_header.data, "data", 4);


    w_header.chunk_size = 16;
    w_header.format_tag = 1;
    w_header.num_of_chanels = 1;
    w_header.sample_rate = parameters.sample_rate;
    w_header.bits_per_sample = 16;
    w_header.bytes_per_sec = (w_header.sample_rate * w_header.bits_per_sample / 8) * w_header.num_of_chanels;
    w_header.bytes_per_sample = w_header.bits_per_sample / 8 * w_header.num_of_chanels;

    double duration_in_sec = ((amount_of_nibbles * parameters.note_length_s) + (amount_of_nibbles * parameters.pause_length_s));
    
    printf("  duration in s: %f\n", duration_in_sec);

    if(duration_in_sec > 3600){
        printf("\nERROR the duration my not be longer than 1h!\n\n");
        exit(EXIT_FAILURE);
    }
    

    uint64_t buffer_size = (uint64_t) (parameters.sample_rate * duration_in_sec);

    uint16_t buffer[buffer_size];
    uint64_t buffer_index = 0;

    uint64_t note_length = (uint64_t) (parameters.sample_rate * parameters.note_length_s);
    uint64_t pause_length = (uint64_t) (parameters.sample_rate * parameters.pause_length_s);

    // go through the characters and turn them into dtmf beeps
    for(uint64_t char_counter = 0; char_counter < amount_of_chars; char_counter++){

        uint8_t nibbles[2];
        // split the byte into the most significant and least significant nibble
        nibbles[0] = (uint8_t) (parameters.data[char_counter] & 0b11110000) >> 4;
        nibbles[1] = (uint8_t) parameters.data[char_counter] & 0b00001111;


        for(uint8_t nibble_index = 0; nibble_index < 2; nibble_index++){
        
            // note sound
            for(uint64_t i = 0; i < note_length; i++){
                buffer[buffer_index] = calculate_dtmf(nibbles[nibble_index], i, parameters.sample_rate);
                buffer_index++;
            }

            // pause
            for(uint64_t i = 0; i < pause_length; i++){
                buffer[buffer_index] = 0;
                buffer_index++;
            }
        }
    }


    w_header.data_length = buffer_size * w_header.bytes_per_sample;
    w_header.file_length = w_header.data_length + sizeof(struct wav_header);

    // print_header(w_header);

    // Writing Wav File to Disk
    FILE *fp = fopen(parameters.file_output, "w");
    fwrite(&w_header, 1, sizeof(struct wav_header), fp);
    fwrite(buffer, 2, buffer_size, fp);

    return 0;
}