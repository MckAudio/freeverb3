/**
 *  Freeverb3 Impulse Response Processor jack plugin
 *
 *  Copyright (C) 2007-2014 Teru Kamogashira
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "fv3_config.h"

#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include <unistd.h>
#include <sys/types.h>

#include <sndfile.hh>
#include <freeverb/irmodel2.hpp>
#include <freeverb/irmodel3.hpp>
#include <fftw3.h>
#include <jack/jack.h>
#include "CArg.hpp"

#ifdef PLUGDOUBLE
typedef fv3::irbase_ IRBASE;
typedef fv3::irmodel2_ IR2;
typedef fv3::irmodel3_ IR3;
typedef fv3::utils_ UTILS;
typedef double pfloat_t;
#else
typedef fv3::irbase_f IRBASE;
typedef fv3::irmodel2_f IR2;
typedef fv3::irmodel3_f IR3;
typedef fv3::utils_f UTILS;
typedef float pfloat_t;
#endif

CArg args;
IRBASE * ir;
SndfileHandle *impulse;
float idb = -5, odb = -25;

void splitLR(pfloat_t * data, pfloat_t * L, pfloat_t * R,
             int singleSize, int channels)
{
  for(int t = 0; t < singleSize; t++)
    {
      L[t] = data[t*channels+0];
      if(channels > 1)
        R[t] = data[t*channels+1];
      else
        R[t] = L[t];
    }
}

jack_port_t *input_left, *input_right;
jack_port_t *output_left, *output_right;
jack_client_t *client;

void vprocess(float * inL, float * inR, float * outL, float * outR, int nframes)
{
#ifdef PLUGDOUBLE
  double *dinL, *dinR, *doutL, *doutR;
  dinL = new double[nframes];
  dinR = new double[nframes];
  doutL = new double[nframes];
  doutR = new double[nframes];

  for(int i = 0;i < nframes;i ++)
    {
      dinL[i] = inL[i]; dinR[i] = inR[i];
    }
  ir->processreplace(dinL, dinR, doutL, doutR, nframes, FV3_IR_SKIP_FILTER);
  for(int i = 0;i < nframes;i ++)
    {
      outL[i] = doutL[i]; outR[i] = doutR[i];
    }
  
  delete[] dinL;
  delete[] dinR;
  delete[] doutL;
  delete[] doutR;
#else
  ir->processreplace(inL, inR, outL, outR, nframes, FV3_IR_SKIP_FILTER);
#endif
}

int process(jack_nframes_t nframes, void *arg)
{
  jack_default_audio_sample_t *inL, *inR, *outL, *outR;
  inL = (jack_default_audio_sample_t*)jack_port_get_buffer(input_left, nframes);
  inR =  (jack_default_audio_sample_t*)jack_port_get_buffer(input_right, nframes);
  outL = (jack_default_audio_sample_t*)jack_port_get_buffer(output_left, nframes);
  outR = (jack_default_audio_sample_t*)jack_port_get_buffer(output_right, nframes);
  vprocess(inL, inR, outL, outR, nframes);
  return 0;
}

void jack_shutdown(void *arg)
{
  exit(1);
}

void help(const char * cmd)
{
  std::fprintf(stderr,
               "Usage: %s [options] ImpulseResponse.wav\n"
               "ImpulseReponse: libsndfile supported file.\n"
               "[[Options]]\n"
               "-m irmodel type\n"
               "\tn MODEL\n"
               "\tdefault, 0 irmodel2 fastest\n"
               "\t3 irmodel3 zero latency\n"
               "-indb Input Fader (arg-5)[dB]\n"
               "-imdb Impulse Fader (arg-25)[dB]\n"
               "-id JACK Unique ID (default/0=pid)\n"
               "[[Example]]\n"
               "%s IR.wav -imdb -5\n"
               "\n",
               cmd, cmd);
}

int main(int argc, char **argv)
{
  std::fprintf(stderr, "Impulse Response Processor for JACK\n");
  std::fprintf(stderr, "<" PACKAGE "-" VERSION ">\n");
  std::fprintf(stderr, "Copyright (C) 2007-2014 Teru Kamogashira\n");
  std::fprintf(stderr, "sizeof(pfloat_t) = %d\n", (int)sizeof(pfloat_t));
  
  if(argc <= 1) help(argv[0]), exit(-1);
  if(args.registerArg(argc, argv) != 0) exit(-1);
  
  impulse = new SndfileHandle(args.getFileArg(0));
  if(impulse->frames() == 0)
    {
      std::fprintf(stderr, "ERROR: open PCM file %s.\n", args.getFileArg(0));
      exit(-1);
    }
  
  long model = args.getLong("-m");
  switch(model)
    {
    case 1:
      std::fprintf(stderr, "MODEL = irmodel3\n");
      ir = new IR3();
      break;
    case 0:
    default:
      std::fprintf(stderr, "MODEL = irmodel2\n");
      ir = new IR2();
      break;
    }
  
  pfloat_t * irStream =
    new pfloat_t[((int)impulse->frames())*impulse->channels()];
  sf_count_t rcount = impulse->readf(irStream, impulse->frames());
  if(rcount != impulse->frames())
    {
      std::fprintf(stderr, "ERROR: readf impulse\n");
      delete[] irStream;
      exit(-1);
    }
  pfloat_t * irL = new pfloat_t[(int)impulse->frames()];
  pfloat_t * irR = new pfloat_t[(int)impulse->frames()];
  splitLR(irStream, irL, irR, impulse->frames(), impulse->channels());

  std::fprintf(stderr, "loading...");
  ir->loadImpulse(irL, irR, impulse->frames());
  delete[] irL;
  delete[] irR;
  delete[] irStream;
  std::fprintf(stderr, "done.\n");
  std::fprintf(stderr, "Size = %ld, Latency = %ld\n", ir->getImpulseSize(), ir->getLatency());
  
  idb += args.getDouble("-indb");
  odb += args.getDouble("-imdb");
  std::fprintf(stderr, "Input %.1f[dB] Impulse %.1f[dB]\n", idb, odb);
  ir->setdry(idb);
  ir->setwet(odb);
  ir->setwidth(1);
  
  char *client_name, *server_name = NULL;
  jack_options_t options = JackNullOption;
  jack_status_t status;
  
  int uid = args.getLong("-id");
  if(uid <= 0) uid = getpid();
  char unique_name[1024];
  std::snprintf(unique_name, 1024, "freeverb3_impulser_%d", uid);
  client_name = unique_name;
  client = jack_client_open(client_name, options, &status, server_name);
  if (client == NULL)
    {
      std::fprintf(stderr, "jack_client_open() failed, status = 0x%2.0x\n", status);
      if (status & JackServerFailed)
        {
          std::fprintf(stderr, "Unable to connect to JACK server\n");
        }
      exit (-1);
    }
  if (status & JackServerStarted)
    std::fprintf(stderr, "JACK server started\n");
  if (status & JackNameNotUnique)
    {
      client_name = jack_get_client_name(client);
      std::fprintf(stderr, "unique name `%s' assigned\n", client_name);
    }
  
  jack_set_process_callback(client, process, 0);
  jack_on_shutdown(client, jack_shutdown, 0);
  input_left = jack_port_register(client, "inputL", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  input_right = jack_port_register(client, "inputR",JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  output_left = jack_port_register(client, "outputL", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  output_right = jack_port_register(client, "outputR", JACK_DEFAULT_AUDIO_TYPE,JackPortIsOutput, 0);
  if(input_left == NULL||input_right == NULL
     ||output_left == NULL||output_right == NULL)
    {
      std::fprintf(stderr, "no more JACK ports available\n");
      exit(-1);
    }
  
  if(jack_activate(client))
    {
      std::fprintf(stderr, "cannot activate client\n");
      exit(-1);
    }

  while(1) sleep(1);   
  jack_client_close(client);
  delete ir;
  exit(0);
}
