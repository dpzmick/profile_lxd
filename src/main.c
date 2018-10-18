#include "app.h"
#include "common.h"
#include "err.h"

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <jack/jack.h>

/* idx to use for things with a bunch of ports in them */
enum {
  SQUARE_OUT = 0,
  PULSE_OUT  = 1,
  RESULT_IN  = 2,
  PORT_COUNT,
};

#if ATOMIC_BOOL_LOCK_FREE!=2
#error "atomic_bool must be lock free!"
#endif

/* All state for this app is global */
static atomic_bool  running;          /* no way to static init explicitly */
static app_t*       app               = NULL;
static jack_port_t* ports[PORT_COUNT] = { NULL, NULL, NULL };

static void
usage(char const * appname)
{
  fprintf(stderr, "Usage: %s: square-out pulse-out result-in\n", appname);
}

/* Responsible for getting and populating the buffers associated with all of our ports */

static int
jack_process_callback(jack_nframes_t nframes, void* arg)
{
  /* REALTIME SAFE */
  (void)arg;
  assert(app); /* super sketchy */

  /* FIXME not clear if it is safe to access these ports and connect these ports
     at the same time. So far, it hasn't crashed anything, so I'm going to
     assume its okay */

  jack_default_audio_sample_t* square =
    (jack_default_audio_sample_t*)jack_port_get_buffer(ports[SQUARE_OUT], nframes);

  jack_default_audio_sample_t* pulse =
    (jack_default_audio_sample_t*)jack_port_get_buffer(ports[PULSE_OUT], nframes);

  jack_default_audio_sample_t const* result =
    (jack_default_audio_sample_t const*)jack_port_get_buffer(ports[RESULT_IN], nframes);

  /* FIXME store the error somewhere */
  int ret = app_poll(app, (size_t)nframes, square, pulse, result);
  return (ret == APP_SUCCESS) ? 0 : 1;
}

/* If a client was too slow, this is triggered.
   FIXME is this triggered if any client is too slow, or just if we are too slow? */

static int
jack_xrun_callback(void* arg)
{
  /* not required to be realtime safe */
  (void)arg;

  printf("xrun, weird....\n");
  return 0;
}

/* FIXME do we care about buffer size changes? */

static void
handle_signal(int signo)
{
  (void)signo;
  assert(signo == SIGINT); /* sketchy as hell */
  atomic_store(&running, false);
}

int
main(int argc, char ** argv)
{
  int ret = 0;
  if (argc != 4) {
    usage(argv[0]);
    return 1;
  }

  char const* connect_port_names[PORT_COUNT] = {
    argv[SQUARE_OUT+1], argv[PULSE_OUT+1], argv[RESULT_IN+1]
  };

  jack_status_t status[1];
  jack_client_t * client = jack_client_open("profile_lxd", JackNoStartServer, status);
  if (!client) {
    fprintf(stderr, "Failed to create jack client status=0x%x\n", status[0]);
    return 1;
  }

  char const*    client_name = jack_get_client_name(client); /* ref to some internal structure */
  jack_nframes_t sample_rate = jack_get_sample_rate(client);
  jack_nframes_t buffer_size = jack_get_buffer_size(client); /* subject to change during processing */
  int            rt          = jack_is_realtime(client);

  printf("Successfully initialized jack client named '%s'.\nProperties:\n", client_name);
  printf("  sample_rate=%u\n  buffer_size=%u\n  realtime=%s\n",
         sample_rate, buffer_size, rt ? "true" : "false");

  /* Create the ports that we need */

  char const*   names[PORT_COUNT] = { "square-out", "pulse-out", "result-in" };
  unsigned long flags[PORT_COUNT] = { JackPortIsOutput, JackPortIsOutput,
                                      JackPortIsInput | JackPortIsTerminal };

  for (size_t i = 0; i < PORT_COUNT; ++i) {
    ports[i] = jack_port_register(client, names[i], JACK_DEFAULT_AUDIO_TYPE, flags[i], 0);
    if (!ports[i]) {
      fprintf(stderr, "Failed to register port '%s'. Bailing out\n", names[i]);
      goto exit;
    }
  }

  /* Setup the audio processing app */
  app = create_app(sample_rate, &ret);
  if (!app) {
    fprintf(stderr, "failed to create app with '%s'\n", app_errstr(ret));
    goto exit;
  }

  /* Set up jack callback handlers */
  ret = jack_set_xrun_callback(client, jack_xrun_callback, NULL);
  if (ret != 0) {
    fprintf(stderr, "failed to set xrun callback ret=%d\n", ret);
    goto exit;
  }

  ret = jack_set_process_callback(client, jack_process_callback, NULL);
  if (ret != 0) {
    fprintf(stderr, "failed to set processing callback ret=%d\n", ret);
    goto exit;
  }

  /* Set running to true before setting up signal handlers */

  atomic_store(&running, true);
  struct sigaction act[1];
  memset(act, 0, sizeof(*act));
  act->sa_handler = handle_signal;
  if (0 != sigaction(SIGINT, act, NULL)) {
    fprintf(stderr, "failed to setup signal handlers with: '%s' (%d)\n", strerror(errno), errno);
    goto exit;
  }

  /* fire up the processing thread */
  ret = jack_activate(client);
  if (ret != 0) {
    fprintf(stderr, "Failed to activate jack, ret=%d\n", ret);
    goto exit;
  }

  /* Connect the ports to the ones the user wants the connected to. Can't be
     done until the client is active. */

  for (size_t i = 0; i < PORT_COUNT; ++i) {
    char const* out = flags[i] & JackPortIsOutput ? jack_port_name(ports[i]) : connect_port_names[i];
    char const* in  = flags[i] & JackPortIsOutput ? connect_port_names[i]    : jack_port_name(ports[i]);

    ret = jack_connect(client, out, in);
    if (ret != 0) fprintf(stderr, "Failed to connect port '%s' to '%s'. Moving along\n", out, in);
  }

  /* Wait for something to shut us down */

  while (atomic_load(&running)) {
    printf("\rnow=%ld", time(NULL));
    fflush(stdout);
    usleep(1000 /*millis */ * 1000 /* seconds */ * 1);
  }
  printf("\n");

exit:
  printf("Shutting down '%s'\n", argv[0]);

  /* Implicitly disconnects all ports associated with the client */
  ret = jack_deactivate(client);
  if (ret != 0) fprintf(stderr, "deactivate failed, ret=%d\n", ret);

  for (size_t i = 0; i < PORT_COUNT; ++i) {
    ret = jack_port_unregister(client, ports[i]);
    if (ret != 0) fprintf(stderr, "Failed to unregister port, continuing. ret=%d\n", ret);
    ports[i] = NULL;
  }

  ret = jack_client_close(client);
  if (ret != 0) {
    fprintf(stderr, "Failed to close jack client, ret=%d\n", ret);
  }

  /* app can be torn down last since it has no idea about anything jack related */
  if (app) destroy_app(app);
}

/* Other interesting jack APIS:
   - jack CPU load estimation might be interesting to poll periodically
   - some info/error reporting callbacks may be defined as well, perhaps we can
     queue the logging and report in the main loop?
*/
