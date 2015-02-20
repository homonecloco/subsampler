/*
 * Copyright 2011-2013 Ricardo Ramirez
 * 
 * **********************************************************************
 *
 * This file is part of subsampler.
 *
 * subsampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * subsampler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CORTEX.  If not, see <http://www.gnu.org/licenses/>.
 * Subsampler uses bits of code from CORTEX (http://cortexassembler.sourceforge.net)
 *
 * **********************************************************************
 */


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include <nucleotide.h>
#include <seq.h>
#include <logger.h>

struct arguments {
    char * file_in;
    char * file_out;
    int quality_offset;
    double probability;
    int append;
    int max_read_length;
    int max_name_lenght;
    char * log_file;
    unsigned int seed;
};
void parse_opt(int key, char *arg, struct arguments *arguments);
void sampler_parse(int argc, char **argv, struct arguments * arg);
//void print_help();

static void print_help() {
    log_and_screen_printf("subsampler [-a] [-f sanger | illumina] [-i input] [-o output] [-l max_read_length] [-n max_name_length] [-p probability] [-q quality_offset] [-s rand_seed] [-h] [-L log_file]\n");
    log_and_screen_printf("\t-a\t Append to an existing file \n");
    log_and_screen_printf("\t-f\t sanger for offset +33, illumina for offset +64.\n");
    log_and_screen_printf("\t-i\t Input from a file. Default stdin.\n");
    log_and_screen_printf("\t-o\t Output to a file. Default stdout.\n");
    log_and_screen_printf("\t-l\t Maximum read length. Default 2000 \n");
    log_and_screen_printf("\t-n\t Maximum name length. Default 1000\n");
    log_and_screen_printf("\t-p\t Probability to pick a read. Default 0.01 (1\%% of the reads will be picked)\n");
    log_and_screen_printf("\t-q\t Quality offset. Default 33. Strictly, not necessery, as the same offset is applied for the in and out\n");
    log_and_screen_printf("\t-s\t Seed for the random number generator. The defualt is a timestamp. Useful if you want to get the same output on different runs.\n");
    log_and_screen_printf("\t-h\t Prints this help\t \n");
    log_and_screen_printf("\t-L\t Logfile. In addition to stderr, the log is printed to this file. \t \n");
    exit(-2);
}

void parse_opt(int key, char *arg, struct arguments *arguments) {
    switch (key) {
        case 'a':
            arguments->append = 1;
        case 'f':
            if (strcmp(arg, "sanger") == 0) {
                arguments->quality_offset = 33;
            } else if (strcmp(arg, "illumina") == 0) {
                arguments->quality_offset = 64;
            } else {
                fprintf(stderr, "[-f sanger | illumina] invalid input format (%s). \n", arg);
                exit(-1);
            }
            break;
        case 'h':
            print_help();

        case 'i':
            arguments->file_in = arg;
            break;
        case 'l':
            arguments->max_read_length = atoi(arg);
            break;
        case 'n':
            arguments->max_name_lenght = atoi(arg);
            break;
        case 'o':
            arguments->file_out = arg;
            break;
        case 'p':
            arguments->probability = atof(arg);
            break;
        case 'q':
            arguments->quality_offset = atoi(arg);
            break;
        case 's':
            arguments->seed = atoi(arg);
            break;
        case 'L':
            arguments->log_file = arg;
        default:
            break;
    }
}

void sampler_parse(int argc, char **argv, struct arguments * arg) {
    opterr = 0;
    int c;

    while ((c = getopt(argc, argv, "af:hi:l:n:o:p:q:s:L")) != -1) {
        parse_opt(c, optarg, arg);
    }
}

const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

const int isFasta(const char* filename) {
    const char *ext = get_filename_ext(filename);
    return strcmp(ext, "fa") == 0 || strcmp(ext, "fasta") == 0;    
}

const int isFastq(const char* filename) {
    const char *ext = get_filename_ext(filename);
    return strcmp(ext, "fq") == 0 || strcmp(ext, "fasta") == 0;
}

int main(int argc, char * argv[]) {
    log_set_screen(stderr);

    if (argc < 2) {
        print_help();
    }

    struct arguments args;
    FILE * in;
    FILE * out;
    Sequence * seq;
    //Defaults
    args.file_out = "-";
    args.file_in = "-";
    args.probability = 0.01;
    args.append = 0;
    args.quality_offset = 33;
    args.max_name_lenght = 1000;
    args.max_read_length = 2000;
    args.seed = (unsigned int) time(NULL);
    args.log_file = NULL;

    sampler_parse(argc, argv, &args);

    if (args.log_file != NULL) {
        log_start(args.log_file);
    }
    log_and_screen_printf("Subsampler\n");
    log_write_timestamp(1);

    if (strcmp(args.file_in, "-") == 0) {
        in = stdin;
    } else {
        in = fopen(args.file_in, "r"); //open file of file names
        if (in == NULL) {
            log_and_screen_printf("Unable to open file %s\n", args.file_in);
            exit(-1);
        }
    }

    if (strcmp(args.file_out, "-") == 0) {
        out = stdout;
    } else {
        out = fopen(args.file_out, "w");
        if (out == NULL) {
            log_and_screen_printf("Unable to open file %s\n", args.file_out);
            exit(-1);
        }
    }
    log_and_screen_printf("Seed\t%d\n", args.seed);
    srand(args.seed);
    seq = sequence_new(args.max_read_length, args.max_name_lenght, args.quality_offset);

    double p;
    unsigned long readed = 0;
    unsigned long printed = 0;

    if (isFastq(args.file_in)) {
        while (read_sequence_from_fastq(in, seq, args.max_read_length)) {
            p = (double) rand() / RAND_MAX;
            readed++;
            if (p < args.probability) {
                sequence_print_fastq(out, seq);
                printed++;
            }
        }

    } else if (isFasta(args.file_in)) {
        boolean full_entry = true;
        while (read_sequence_from_fasta(in, seq, args.max_read_length, true, &full_entry, 0)) {
            p = (double) rand() / RAND_MAX;
            readed++;
            if (p < args.probability) {
                sequence_print_fasta(out, seq);
                printed++;
            }
        }

    } else {
        log_and_screen_printf("Unknown file type detected %s.  Expected file with extension {\"fa\",\"fasta\",\"fq\",\"fastq\"}\n", args.file_in);
        exit(-1);
    }


    log_and_screen_printf("Readed\t%ld\nPrinted\t%ld\nProbability\t%f\n", readed, printed, ((double) printed / (double) readed))
            ;
    log_write_timestamp(1);
    log_and_screen_printf("DONE\n");

    free_sequence(&seq);
    if (in != stdin) {
        fclose(in);
    }
    if (out != stdout) {
        fclose(out);
    }




    return 0;
}
