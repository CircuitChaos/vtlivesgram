# VT Live Spectrogram – source code structure

If you want to contribute to the code, here's the overall program structure which might make it easier.

# Entry point and main loop

Program entry point is in *main.cpp*. It creates an instance of two classes: CView (user interface) and 
CSource (for providing magnitudes to be presented by the user interface) and starts a select()-based loop 
to watch for readability on two file descriptors: X server connection (xfd) and stdin (sfd).

If X server connection is readable, then CView::evt() is called to read and process all X server events. 
It returns a bit mask with user interface events, which can contain one or more of the following flags:

* CView::EVT_TERMINATE – window has been closed, program has to terminate
* CView::EVT_WIDTH_CHANGED – window width been changed, need to pass new width to the CSource instance
* CView::EVT_SPEED_CHANGED – waterfall speed has been changed, need to pass new speed to the CSource instance
* CView::EVT_NEXT_FFT_WINDOW – next FFT window has been requested, need to pass it to the CSource instance

If standard input is readable, then CSource::read() is called, which can yield three results:

* End-of-file on input (method returns false) – program has to terminate
* No result (empty output vector) – it means that more input data is needed and nothing is done
* One or more rows for the user interface – they are passed to the CView instance along with the input sample 
rate, last timestamp and current FFT window name

# Data source (CSource)

The most important method is CSource::read(). It reads a block of data from the standard input, feeds it to 
the instance of CVtParser class, which parses it and returns raw samples and metadata (sample rate and 
timestamp), and then – as long as the number of samples accumulated is enough (at least sample rate divided 
by the speed) – calls operator() from the CFft instance to produce output magnitudes.

# User interface (CView)

This class is responsible for displaying data provided by CSource and for interacting with the user. In its 
constructor, a connection to the X server is initialized and the program window is created. It also sets the 
window name, creates an atom to be read when the window manager closes the window, configures window event 
masks, reads the initial window size and initializes images, as for efficiency everything is drawn on images 
(XImage instances), which are put into the window using XPutImage().

There are two images – main image and status bar image – because status bar might be refreshed much more 
frequently than the main image (when the mouse is moved over the window and current frequency and magnitude 
on the status bar needs to be updated).

After the constructor, everything is done in the main event method – CView::evt(). This method reacts to 
the following X events:

* MotionNotify (mouse has been moved)
* ButtonPress (mouse button has been pressed or mouse wheel has been turned)
* Expose (window has been exposed)
* ConfigureNotify (window has been resized)
* MapNotify (window has been mapped)
* ClientMessage (window has been closed by the window manager)

There are many private methods in the CView class, but I believe they're more or less self-explanatory.

# Other classes

There are other classes which are used by two main classes mentioned above.

## CVtParser

Parses input in vlfrx-tools format, performs checks and outputs metadata (sample rate and timestamp) and 
raw samples.

## CFft

This is the core part of the program. It uses *libfftw* to perform FFT calculations. Its main 
method is CFft::operator(). When called, it checks the output width and calculates FFT width 
from it (multiplies it by two), as the output contains only relevant number of frequency bins (half 
of the FFT width). If the FFT width has been changed (or it's the first call), it calls CFft::init() 
to reinitialize input and output buffers, recreate FFT window and FFTW plan. Then it creates a vector 
to store magnitudes, splits input samples into a number of blocks, each containing a number of samples 
equal to the FFT width, applies a FFT window on them and performs one or more FFT operations, calling 
fftw_execute(). After each FFT round, it parses the FFT output and obtains magnitudes from the complex 
samples returned by the FFT by calculating the square root of the sum of squares of the real and 
imaginary parts (sqrt(re² + im²)). These magnitudes are then accumulated in the magnitude vector.

After FFT is done, any number of remaining samples are ignored (see „Known issues” above) and 
magnitudes are scaled, so they can be presented in dBFS. Scaling is done by dividing each magnitude 
by the scale factor (FFT width multiplied by the number of iterations) and calculating 20 common 
logarithms on them (20 * log10(magnitude / (width * iterations)). Then the resulting floating-point 
value is clipped to the safe range (0.0 – -650.0), converted to the fixed-point return value and put 
in the output buffer.

CFft also has nextWindow() and getWindowName() methods for handling different FFT windows. All supported 
window types are implemented in the private windowFunction() method.

## CSoxPal

Converts magnitude in dBFS (from 0 to -120) to RGB values of colors used by spectrogram generated by 
*sox*, as I like its palette.

## CFont

Stores a DOS font which is used by CView to draw characters on the status bar.

## SXResources

This is a structure, not a class, although it contains a constructor and destructor. It contains all X resources used by CView. It has been separated, because CView's constructor does many things and if it fails and throws an exception, the resources wouldn't be freed by the program (although it wouldn't be a big deal, because the program would exit anyway).
