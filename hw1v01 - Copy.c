#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define HEADER_MIN_SIZE 44
#define EXIT_OK  0
#define EXIT_ERR 1

static int read_n_bytes(unsigned char *buf, size_t n) {
    size_t i;
    for (i = 0; i < n; ++i) {
        int c = getchar();
        if (c == EOF) return 1;
        buf[i] = (unsigned char)c;
    }
    return 0;
}

static unsigned int read_le_u32(const unsigned char *p) {
    return (unsigned int)p[0]
         | ((unsigned int)p[1] << 8)
         | ((unsigned int)p[2] << 16)
         | ((unsigned int)p[3] << 24);
}

static unsigned int read_le_u16(const unsigned char *p) {
    return (unsigned int)p[0]
         | ((unsigned int)p[1] << 8);
}

static void write_le_u32(unsigned char *p, unsigned int v) {
    p[0] = (unsigned char)(v & 0xFFu);
    p[1] = (unsigned char)((v >> 8) & 0xFFu);
    p[2] = (unsigned char)((v >> 16) & 0xFFu);
    p[3] = (unsigned char)((v >> 24) & 0xFFu);
}

static void write_le_u16(unsigned char *p, unsigned int v) {
    p[0] = (unsigned char)(v & 0xFFu);
    p[1] = (unsigned char)((v >> 8) & 0xFFu);
}

static void write_buffer(const unsigned char *buf, size_t n) {
    size_t i;
    for (i = 0; i < n; ++i) putchar(buf[i]);
}

static void copy_rest_of_stream(void) {
    int c;
    while ((c = getchar()) != EOF) putchar(c);
}

static int read_and_validate_header(unsigned char header[HEADER_MIN_SIZE],
                                    int print_info,
                                    unsigned int *size_of_file,
                                    unsigned int *size_of_format_chunk,
                                    unsigned int *wave_type_format,
                                    unsigned int *mono_stereo,
                                    unsigned int *sample_rate,
                                    unsigned int *bytes_per_sec,
                                    unsigned int *block_align,
                                    unsigned int *bits_per_sample,
                                    unsigned int *size_of_data_chunk) {
    unsigned char *p;

    if (read_n_bytes(header, HEADER_MIN_SIZE) != 0) {
        fprintf(stderr, "Error! insufficient data\n");
        return 1;
    }

    if (header[0] != 'R' || header[1] != 'I' ||
        header[2] != 'F' || header[3] != 'F') {
        fprintf(stderr, "Error! \"RIFF\" not found\n");
        return 1;
    }

    *size_of_file = read_le_u32(header + 4);
    if (print_info) printf("size of file: %u\n", *size_of_file);

    if (header[8] != 'W' || header[9] != 'A' ||
        header[10] != 'V' || header[11] != 'E') {
        fprintf(stderr, "Error! \"WAVE\" not found\n");
        return 1;
    }

    if (header[12] != 'f' || header[13] != 'm' ||
        header[14] != 't' || header[15] != ' ') {
        fprintf(stderr, "Error! \"fmt \" not found\n");
        return 1;
    }

    *size_of_format_chunk = read_le_u32(header + 16);
    if (print_info) printf("size of format chunk: %u\n", *size_of_format_chunk);
    if (*size_of_format_chunk != 16u) {
        fprintf(stderr, "Error! size of format chunk should be 16\n");
        return 1;
    }

    *wave_type_format = read_le_u16(header + 20);
    if (print_info) printf("WAVE type format: %u\n", *wave_type_format);
    if (*wave_type_format != 1u) {
        fprintf(stderr, "Error! WAVE type format should be 1\n");
        return 1;
    }

    *mono_stereo = read_le_u16(header + 22);
    if (print_info) printf("mono/stereo: %u\n", *mono_stereo);
    if (!(*mono_stereo == 1u || *mono_stereo == 2u)) {
        fprintf(stderr, "Error! mono/stereo should be 1 or 2\n");
        return 1;
    }

    *sample_rate = read_le_u32(header + 24);
    if (print_info) printf("sample rate: %u\n", *sample_rate);

    *bytes_per_sec = read_le_u32(header + 28);
    if (print_info) printf("bytes/sec: %u\n", *bytes_per_sec);

    *block_align = read_le_u16(header + 32);
    if (print_info) printf("block alignment: %u\n", *block_align);

    *bits_per_sample = read_le_u16(header + 34);
    if (print_info) printf("bits/sample: %u\n", *bits_per_sample);

    if (*bytes_per_sec != (*sample_rate) * (*block_align)) {
        fprintf(stderr, "Error! bytes/second should be sample rate x block alignment\n");
        return 1;
    }

    if (!(*bits_per_sample == 8u || *bits_per_sample == 16u)) {
        fprintf(stderr, "Error! bits/sample should be 8 or 16\n");
        return 1;
    }

    if (*block_align != (*bits_per_sample / 8u) * (*mono_stereo)) {
        fprintf(stderr, "Error! block alignment should be bits per sample / 8 x mono/stereo\n");
        return 1;
    }

    p = header + 36;
    if (p[0] != 'd' || p[1] != 'a' || p[2] != 't' || p[3] != 'a') {
        fprintf(stderr, "Error! \"data\" not found\n");
        return 1;
    }

    *size_of_data_chunk = read_le_u32(header + 40);
    if (print_info) printf("size of data chunk: %u\n", *size_of_data_chunk);

    return 0;
}

static int handle_info(void) {
    unsigned char header[HEADER_MIN_SIZE];
    unsigned int size_of_file, size_of_format_chunk, wave_type_format;
    unsigned int mono_stereo, sample_rate, bytes_per_sec;
    unsigned int block_align, bits_per_sample, size_of_data_chunk;

    if (read_and_validate_header(header, 1,
                                 &size_of_file,
                                 &size_of_format_chunk,
                                 &wave_type_format,
                                 &mono_stereo,
                                 &sample_rate,
                                 &bytes_per_sec,
                                 &block_align,
                                 &bits_per_sample,
                                 &size_of_data_chunk) != 0)
        return EXIT_ERR;

    unsigned int i;
    for (i = 0; i < size_of_data_chunk; ++i) {
        int c = getchar();
        if (c == EOF) {
            fprintf(stderr, "Error! insufficient data\n");
            return EXIT_ERR;
        }
    }

    int extra = getchar();
    if (extra != EOF) {
        fprintf(stderr, "Error! bad file size (found data past the expected end of file)\n");
        return EXIT_ERR;
    }

    return EXIT_OK;
}

static int handle_rate(double factor) {
    unsigned char header[HEADER_MIN_SIZE];
    unsigned int size_of_file, size_of_format_chunk, wave_type_format;
    unsigned int mono_stereo, sample_rate, bytes_per_sec;
    unsigned int block_align, bits_per_sample, size_of_data_chunk;

    if (read_and_validate_header(header, 0,
                                 &size_of_file,
                                 &size_of_format_chunk,
                                 &wave_type_format,
                                 &mono_stereo,
                                 &sample_rate,
                                 &bytes_per_sec,
                                 &block_align,
                                 &bits_per_sample,
                                 &size_of_data_chunk) != 0)
        return EXIT_ERR;

    unsigned int new_sr  = (unsigned int)(sample_rate * factor + 0.5);
    unsigned int new_bps = (unsigned int)(bytes_per_sec * factor + 0.5);

    write_le_u32(header + 24, new_sr);
    write_le_u32(header + 28, new_bps);

    write_buffer(header, HEADER_MIN_SIZE);

    unsigned int i;
    for (i = 0; i < size_of_data_chunk; ++i) {
        int c = getchar();
        if (c == EOF) return EXIT_ERR;
        putchar(c);
    }

    copy_rest_of_stream();
    return EXIT_OK;
}

static int handle_channel(int keep) {
    unsigned char header[HEADER_MIN_SIZE];
    unsigned int size_of_file, size_of_format_chunk, wave_type_format;
    unsigned int mono_stereo, sample_rate, bytes_per_sec;
    unsigned int block_align, bits_per_sample, size_of_data_chunk;

    if (read_and_validate_header(header, 0,
                                 &size_of_file,
                                 &size_of_format_chunk,
                                 &wave_type_format,
                                 &mono_stereo,
                                 &sample_rate,
                                 &bytes_per_sec,
                                 &block_align,
                                 &bits_per_sample,
                                 &size_of_data_chunk) != 0)
        return EXIT_ERR;

    if (mono_stereo == 1u) {
        write_buffer(header, HEADER_MIN_SIZE);
        unsigned int i;
        for (i = 0; i < size_of_data_chunk; ++i) {
            int c = getchar();
            if (c == EOF) return EXIT_ERR;
            putchar(c);
        }
        copy_rest_of_stream();
        return EXIT_OK;
    }

    write_le_u16(header + 22, 1u);
    block_align = bits_per_sample / 8u;
    bytes_per_sec = sample_rate * block_align;
    write_le_u16(header + 32, block_align);
    write_le_u32(header + 28, bytes_per_sec);
    size_of_data_chunk /= 2u;
    write_le_u32(header + 40, size_of_data_chunk);

    write_buffer(header, HEADER_MIN_SIZE);

    unsigned int bytes_per_sample = bits_per_sample / 8u;
    unsigned char frame_buf[8];
    while (1) {
        unsigned int j;
        for (j = 0; j < bytes_per_sample * 2; ++j) {
            int c = getchar();
            if (c == EOF) {
                copy_rest_of_stream();
                return EXIT_OK;
            }
            frame_buf[j] = (unsigned char)c;
        }
        unsigned char *selected = keep == 0 ? frame_buf : frame_buf + bytes_per_sample;
        for (j = 0; j < bytes_per_sample; ++j) putchar(selected[j]);
    }
}

static int handle_volume(double factor) {
    unsigned char header[HEADER_MIN_SIZE];
    unsigned int size_of_file, size_of_format_chunk, wave_type_format;
    unsigned int mono_stereo, sample_rate, bytes_per_sec;
    unsigned int block_align, bits_per_sample, size_of_data_chunk;

    if (read_and_validate_header(header, 0,
                                 &size_of_file,
                                 &size_of_format_chunk,
                                 &wave_type_format,
                                 &mono_stereo,
                                 &sample_rate,
                                 &bytes_per_sec,
                                 &block_align,
                                 &bits_per_sample,
                                 &size_of_data_chunk) != 0)
        return EXIT_ERR;

    write_buffer(header, HEADER_MIN_SIZE);

    unsigned int bytes_per_sample = bits_per_sample / 8u;
    unsigned int total_samples = size_of_data_chunk / bytes_per_sample;

    unsigned int s;
    for (s = 0; s < total_samples; ++s) {
        if (bits_per_sample == 8u) {
            int c = getchar();
            if (c == EOF) return EXIT_ERR;
            double scaled = c * factor;
            if (scaled < 0) scaled = 0;
            if (scaled > 255) scaled = 255;
            putchar((unsigned char)(scaled + 0.5));
        } else {
            unsigned char buf[2];
            if (read_n_bytes(buf, 2) != 0) return EXIT_ERR;
            int val = (signed short)((buf[1] << 8) | buf[0]);
            double scaled = val * factor;
            if (scaled < -32768) scaled = -32768;
            if (scaled >  32767) scaled =  32767;
            int out = (int)lrint(scaled);
            putchar(out & 0xFF);
            putchar((out >> 8) & 0xFF);
        }
    }

    copy_rest_of_stream();
    return EXIT_OK;
}

static void mysound(int dur, int sr, double fm, double fc, double mi, double amp) {
    long total = dur * sr;
    for (long n = 0; n < total; ++n) {
        double t = (double)n / sr;
        double v = amp * sin(2.0 * M_PI * fc * t - mi * sin(2.0 * M_PI * fm * t));
        if (v < -32768) v = -32768;
        if (v > 32767)  v = 32767;
        int s = (int)lrint(v);
        putchar(s & 0xFF);
        putchar((s >> 8) & 0xFF);
    }
}

static int handle_generate(int argc, char **argv) {
    int dur = 3, sr = 44100;
    double fm = 2.0, fc = 1500.0, mi = 100.0, amp = 30000.0;

    for (int i = 2; i < argc; ++i) {
        if (!strcmp(argv[i], "--dur")) dur = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--sr")) sr = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--fm")) fm = atof(argv[++i]);
        else if (!strcmp(argv[i], "--fc")) fc = atof(argv[++i]);
        else if (!strcmp(argv[i], "--mi")) mi = atof(argv[++i]);
        else if (!strcmp(argv[i], "--amp")) amp = atof(argv[++i]);
        else return EXIT_ERR;
    }

    unsigned char header[HEADER_MIN_SIZE];
    header[0]='R';header[1]='I';header[2]='F';header[3]='F';
    unsigned int data_size = dur * sr * 2u;
    write_le_u32(header+4,36u+data_size);
    header[8]='W';header[9]='A';header[10]='V';header[11]='E';
    header[12]='f';header[13]='m';header[14]='t';header[15]=' ';
    write_le_u32(header+16,16u);
    write_le_u16(header+20,1u);
    write_le_u16(header+22,1u);
    write_le_u32(header+24,(unsigned int)sr);
    write_le_u32(header+28,(unsigned int)sr*2u);
    write_le_u16(header+32,2u);
    write_le_u16(header+34,16u);
    header[36]='d';header[37]='a';header[38]='t';header[39]='a';
    write_le_u32(header+40,data_size);

    write_buffer(header, HEADER_MIN_SIZE);
    mysound(dur, sr, fm, fc, mi, amp);
    return EXIT_OK;
}

int main(int argc, char **argv) {
    if (argc < 2) return EXIT_ERR;

    if (!strcmp(argv[1], "info")) return handle_info();
    if (!strcmp(argv[1], "rate")) return handle_rate(atof(argv[2]));
    if (!strcmp(argv[1], "channel")) return handle_channel(!strcmp(argv[2], "right"));
    if (!strcmp(argv[1], "volume")) return handle_volume(atof(argv[2]));
    if (!strcmp(argv[1], "generate")) return handle_generate(argc, argv);

    return EXIT_ERR;
}
