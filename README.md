# VT Live Spectrogram

A tool to display live spectrogram data from *vlfrx-tools* output.

## Objective

I needed a software to display live spectrogram data on Raspberry Pi 4. The task of finding one 
proved to be harder than I expected.

* There's *baudline*, but it's binary-only and there's no ARM port.

* There's *glfer*, but it's unmaintained, still uses OSS and turned out to be pretty unstable on Raspberry.

* There's *vtsgram*, which generates nice-looking spectrograms with *sox*, but it's not live.

* There's *fldigi*, but it's very CPU-intensive.

All other software I found is only for Windows.

My **vtlivesgram** should run on any Linux system (not only Raspberry Pi 4) and might fill this gap.

## Dependencies

**vtlivesgram** expects input to be in format produced by vlfrx-tools, so first you need to download 
and install them: http://abelian.org/vlfrx-tools/.

Then, you'll need some programs and libraries (Debian package names):

* scons
* g++
* libfftw3-dev
* libboost-dev
* libx11-dev
* libstdc++-dev

The program also needs at least 24-bit color display, but it shouldn't be a problem nowadays.

# Compilation

After you install everything, just issue **scons** in the project directory and program should be built. 
Then run **sudo scons install** to install the program in **/usr/local/bin** (or just copy **build/vtlivesgram** 
executable to where you want it).

# Usage

## CLI interface

The program expects to read data produced by vlfrx-tools on its standard input. To test it, you 
can issue:

`vtgen -t -r 48000 -s a=1,f=5000 | vtlivesgram`

It will produce a sine wave at 5 kHz sampled at 48 kHz and feed it to vtlivesgram.

To plot data from .wav file, use:

`vtwavex file.wav | vtcat -- -:1 - | vtlivesgram`

To plot data from ALSA (sound card), use the following command. It will take some time before data starts 
to appear, as *vtcard* needs to calibrate first:

`vtcard -r 48000 | vtcat -- -:1 - | vtlivesgram`

You can provide default speed as the first argument of *vtlivesgram*, for example to plot 5 rows per 
second, use:

`… | vtlivesgram 5.0`

There are some limitations on the input data format though:

* Only single-channel data is supported
* Only float8 (double) sample format is supported
* Timestamps in input data must be absolute, not relative
* Sample rate cannot change mid-stream

These limitations aren't inherent and can be dealt with if there's a need for it – I just didn't see the 
need, so I didn't code it.

## User interface

When **vtlivesgram** starts, a window is created and it's split into three parts:

* Top part – shows the spectrum of the signal
* Middle part – shows the waterfall
* Bottom part – shows the status bar

Waterfall speed can be changed by rotating the mouse wheel. If you resize the window (make it wider), 
the speed might decrease, as there's a maximum speed for a given window size and sample rate – if it's 
exceeded, then there wouldn't be enough samples to calculate the needed number of frequency bins.

When you move the mouse pointer across the window, a status bar will show you the frequency at given 
column and current signal magnitude in dBFS (current, not the one pointed-to by the mouse – vertical 
position of the mouse pointer is not used). Status bar also shows the current timestamp read from the 
input.

To cycle through the available FFT windows (Hanning, rectangular, cosine, Hamming, Blackman, Nuttall), 
press the left mouse button. To freeze (hold) and unfreeze (unhold) the input (for example to make a 
screenshot later), press the right mouse button. Do not resize the frozen window, as the content will 
disappear.

Program terminates either when there's end-of-file on the input (for example when reading from a file) 
or when the window is closed.

Program can also be handled with the keyboard. Supported keys:

* **q**: quit
* **+**: increase speed
* **-**: decrease speed
* **Backspace**: reset speed (set to 1.0)
* **Space**: hold or unhold
* **w**: cycle through FFT windows

# Known issues

The program is still in its alpha stage and there are already some issues that I know of.

* Speed limitations are not applied to the speed specified on the command line. If you specify a speed 
that is too large, you'll either get a full-white spectrogram (with all frequency bins set to 0 dBFS), 
or an error „Trying to read zero samples”.
* When the program window is resized, the waterfall isn't rescaled – it's cleared and starts from the 
beginning. Maybe it would be better to rescale it.
* When doing FFT, the FFT width is adjusted to the number of frequency bins (program window width). Input 
samples (their count depends on the selected waterfall speed) are split into blocks, each containing a 
number of samples for the given FFT width. If the division isn't complete, then there's a number of 
samples which aren't used. I don't know what should I do with them – I tried padding them with zeroes 
to the needed FFT width and applying either a full window on them, or a reduced one (reduced to the 
number of samples), but it didn't yield good results. An advice from someone more experienced with DSP 
than I am (I have very little experience – this program is my first practical approach to DSP) will be 
appreciated.

# Contributions

If you want to contribute to the code, start by reading DEVEL.md file. It contains overall program structure 
and high-level description.

# Contact

You can reach me either through Git issue tracking system, or via email: circuitchaos (at) interia (dot) com.
