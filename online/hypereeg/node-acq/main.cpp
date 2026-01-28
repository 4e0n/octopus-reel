/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2026 Barkin Ilhan

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.

 Contact info:
 E-Mail:  barkin@unrlabs.org
 Website: http://icon.unrlabs.org/staff/barkin/
 Repo:    https://github.com/4e0n/
*/

/* This is the HyperEEG "Acquisition Daemon" Node.. a.k.a. the "Master Node".
 * It acquires from multiple (USB) EEG amps, and stereo audio@48Ksps from the
 * alsa2 audio input device designated within the Octopus HyperEEG box,
 * in a syncronized fashion, synchronizes them altogether and streams to any
 * numer of connected clients via IP:commandPort:streamPort.
 * The settings are fetched by defaults from ~/.octopus-reel/hypereeg.conf
 * which is the common/single config file for all types of HyperEEG clients.
 */

#include <QCoreApplication>
#include "../common/globals.h"
#ifdef EEMAGINE
#include <eemagine/sdk/wrapper.cc>
#endif
#include "acqdaemon.h"

int main(int argc,char *argv[]) {
 QCoreApplication app(argc,argv);
 AcqDaemon acqDaemon;

 omp_diag();

 if (acqDaemon.initialize()) {
  qCritical("node-acq: <FatalError> Failed to initialize Octopus-ReEL EEG Hyperacquisition daemon node.");
  return 1;
 }

 return app.exec();
}

/*
#include <QSocketNotifier>
#include <csignal>
#include <atomic>
#include <unistd.h>   // pipe(), read(), write()
#include "audioamp.h" // your AudioAmp

static int sigPipeFd[2] = {-1, -1};
static std::atomic<bool> g_stop{false};

// POSIX signal handler: write 1 byte to the pipe (async-signal-safe)
extern "C" void on_unix_signal(int) {
    g_stop.store(true, std::memory_order_relaxed);
    char ch = 1;
    ::write(sigPipeFd[1], &ch, 1);  // ignore EAGAIN
}

static void setup_unix_signal_handlers() {
    ::pipe(sigPipeFd);
    // make pipe non-blocking if you like
    struct sigaction sa{};
    sa.sa_handler = on_unix_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT,  &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    // 1) Install SIGINT/SIGTERM handlers
    setup_unix_signal_handlers();

    // 2) Your objects
    AudioAmp audio;
    audio.init();                 // alloc ring, reset state
    if (audio.initAlsa()) {
        audio.start();
    }

    // 3) Ensure cleanup on normal quit as well
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&](){
        audio.stop();             // calls snd_pcm_drop/close inside
    });

    // 4) QSocketNotifier watches the read end of the pipe.
    QSocketNotifier sn(sigPipeFd[0], QSocketNotifier::Read, &app);
    QObject::connect(&sn, &QSocketNotifier::activated, [&](int){
        // drain the pipe
        char buf[64];
        while (::read(sigPipeFd[0], buf, sizeof(buf)) > 0) {}
        // request graceful shutdown
        if (g_stop.load(std::memory_order_relaxed)) {
            // stop audio (safe: we're on the Qt main thread)
            audio.stop();
            app.quit();
        }
    });

    // 5) Run Qt event loop
    return app.exec();
}
*/
