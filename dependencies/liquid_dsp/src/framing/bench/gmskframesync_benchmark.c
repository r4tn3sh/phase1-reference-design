/*
 * Copyright (c) 2007 - 2014 Joseph Gaeddert
 *
 * This file is part of liquid.
 *
 * liquid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * liquid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with liquid.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <math.h>
#include <assert.h>
#include "liquid.h"

typedef struct {
    unsigned char * header;
    unsigned char * payload;
    unsigned int num_frames_tx;
    unsigned int num_frames_detected;
    unsigned int num_headers_valid;
    unsigned int num_payloads_valid;
} framedata;

static int callback(unsigned char *  _header,
                    int              _header_valid,
                    unsigned char *  _payload,
                    unsigned int     _payload_len,
                    int              _payload_valid,
                    framesyncstats_s _stats,
                    void *           _userdata)
{
    //printf("callback invoked\n");
    framedata * fd = (framedata*) _userdata;

    // increment number of frames detected
    fd->num_frames_detected++;

    // check if header is valid
    if (_header_valid)
        fd->num_headers_valid++;

    // check if payload is valid
    if (_payload_valid)
        fd->num_payloads_valid++;

    return 0;
}

// benchmark regular frame synchronizer with short frames; effectively
// test acquisition complexity
void benchmark_gmskframesync(struct rusage *     _start,
                             struct rusage *     _finish,
                             unsigned long int * _num_iterations)
{
    *_num_iterations /= 128;
    unsigned long int i;

    // options
    unsigned int payload_len = 8;       // length of payload (bytes)
    crc_scheme check = LIQUID_CRC_32;   // data validity check
    fec_scheme fec0  = LIQUID_FEC_NONE; // inner forward error correction
    fec_scheme fec1  = LIQUID_FEC_NONE; // outer forward error correction
    float SNRdB = 30.0f;                // SNR

    // derived values
    float nstd  = powf(10.0f, -SNRdB/20.0f);

    // create gmskframegen object
    gmskframegen fg = gmskframegen_create();

    // frame data
    unsigned char header[14];
    unsigned char payload[payload_len];
    // initialize header, payload
    for (i=0; i<14; i++)
        header[i] = i;
    for (i=0; i<payload_len; i++)
        payload[i] = rand() & 0xff;
    framedata fd = {header, payload, 0, 0, 0, 0};

    // create gmskframesync object
    gmskframesync fs = gmskframesync_create(callback,(void*)&fd);
    //gmskframesync_print(fs);

    // generate the frame
    gmskframegen_assemble(fg, header, payload, payload_len, check, fec0, fec1);
    gmskframegen_print(fg);
    unsigned int frame_len = gmskframegen_getframelen(fg);
    float complex frame[frame_len];
    int frame_complete = 0;
    unsigned int n=0;
    while (!frame_complete) {
        assert(n < frame_len);
        frame_complete = gmskframegen_write_samples(fg, &frame[n]);
        n += 2;
    }
    // add some noise
    for (i=0; i<frame_len; i++)
        frame[i] += nstd*(randnf() + _Complex_I*randnf());

    // 
    // start trials
    //
    getrusage(RUSAGE_SELF, _start);
    for (i=0; i<(*_num_iterations); i++) {
        gmskframesync_execute(fs, frame, frame_len);
    }
    getrusage(RUSAGE_SELF, _finish);

    // print results
    fd.num_frames_tx = *_num_iterations;
    printf("  frames detected/header/payload/transmitted:   %6u / %6u / %6u / %6u\n",
            fd.num_frames_detected,
            fd.num_headers_valid,
            fd.num_payloads_valid,
            fd.num_frames_tx);

    // destroy objects
    gmskframegen_destroy(fg);
    gmskframesync_destroy(fs);
}

// benchmark regular frame synchronizer with noise; essentially test
// complexity when no signal is present
void benchmark_gmskframesync_noise(struct rusage *     _start,
                                   struct rusage *     _finish,
                                   unsigned long int * _num_iterations)
{
    *_num_iterations /= 400;
    unsigned long int i;

    // options
    float SNRdB = 20.0f;                // SNR

    // derived values
    float nstd  = powf(10.0f, -SNRdB/20.0f);

    // create frame synchronizer
    gmskframesync fs = gmskframesync_create(NULL, NULL);

    // allocate memory for noise buffer and initialize
    unsigned int num_samples = 1024;
    float complex y[num_samples];
    for (i=0; i<num_samples; i++)
        y[i] = nstd*(randnf() + randnf()*_Complex_I)*M_SQRT1_2;

    // 
    // start trials
    //
    getrusage(RUSAGE_SELF, _start);
    for (i=0; i<(*_num_iterations); i++) {
        // push samples through synchronizer
        gmskframesync_execute(fs, y, num_samples);
    }
    getrusage(RUSAGE_SELF, _finish);

    // scale result by number of samples in buffer
    *_num_iterations *= num_samples;

    // destroy framing objects
    gmskframesync_destroy(fs);
}

