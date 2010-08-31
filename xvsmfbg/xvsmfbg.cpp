/*
 *   Written by Peter Schmidt-Nielsen
 * Copyleft, 2010. All wrongs reserved.
 *         (Public domain)
 */

#include "xvsmfbg.h"

// This program is hardcoded to skip the xwd header in each frame, by skipping the first IMG_OFFSET bytes in the buffer.
// This program also assumes that the input file uses 1 byte per pixel, and 480x240. (i.e., -screen 0 480x240x8 was passed to Xvfb)
// TODO: Respect the header!!
//
// This format is unfortunately hard to Google for documentation about.
// For epsilon more documentation of the format, see: http://www.martinreddy.net/gfx/2d/XWD.txt
// Also, apt-get source x11-apps will get xwud, a program which uses this format.
#define IMG_OFFSET (0xCA0)

#define ASSUMED_WIDTH  480
#define ASSUMED_HEIGHT 240

volatile bool keep_running = true;

// This is registered as SIGINT's handler,
// then a while(keep_running) loop is entered.
void stop_running(int sig) {
	cerr << " Ctrl+C caught: quitting." << endl;
	keep_running = false;
}

// Takes a portion of frame buffer, and writes it to the output stream after converting it to bit packed monochrome format.
// Bitpacked monochrome currently defines the most siginificant bit to be the leading (or leftmost bit).
int write_bits( unsigned char* img_buffer, int length ) {
	static unsigned char* buffer = NULL;
	static int cached_length = 0;

	// Test if we already have a cached memory buffer suitable for staging the output.
	// If not, create one and cache it.
	if (buffer == NULL || cached_length != length) {
		buffer = (unsigned char*) realloc(buffer, (length+7) / 8);
		cached_length = length;
	}

	unsigned char* current_output_byte = buffer;

	// Iterate through each byte of the input image.
	for (int ii=0; ii < ((length+7) / 8); ii++) {
		unsigned char build = 0;

		// Build a byte of output, one bit at a time
		for (int jj=0; jj<8; jj++) {
			build <<= 1;

			// Dithering mode: Take low order bit of color.
			// I am actually shocked this worked at all.
			// You'd expect low order bit to just be too noisy, right?
			build ^= (*img_buffer) & 1;
			img_buffer++;
		}

		// For safety, the last pixel of every line must be blanked.
		// Note that ASSUMED_WIDTH/8 is the number of bytes per line.
		if ( ((ii+1) % (ASSUMED_WIDTH/8)) == 0 ) {
			build &= 0xfe;
		}

		*current_output_byte = build;
		current_output_byte++;
	}

	//cerr << "Sending frame of size: " << ((length+7) / 8) << endl;

	// Write the output to stdout, which is the current output stream.
	return write(1, buffer, (length+7) / 8);
}

int main(int argc, char** argv) {

	// By default, run at at most 1.666 frames per second.
	// This has been hand tuned to sync with the rate of sending frames 480x240 of 115200 bits over the serial port at 921600 baud.
	// This sucks! TODO: Once flow control is implemented, simply call select(2) on the serial port until you can send another frame.
	// TODO: Read this from a configuration file. 
	long long usleep_duration = 600000;

	// Start by parsing the string printed out by: Xvfb -shmem
	// An example such string would be: "screen 0 shmid 2949151"
	cerr << "Parsing Xvfb's output..." << endl;

	string shmem_str;

	string null;
	cin >> null;      // Ignore "screen"
	cin.ignore(1);    // Ignore " "
	cin >> null;      // Ignore screen number
	cin.ignore(1);    // Ignore " "
	cin >> null;      // Ignore "shmid"
	cin.ignore(1);    // Ignore " "
	cin >> shmem_str; // Read in the crucial shared memory ID number.

	cerr << "Connecting to shared memory ID: " << shmem_str << endl;

	int shmid;

	// Convert shmem_str to an integer, and store it in shmid.
	stringstream ss;
	ss << shmem_str;
	ss >> shmid;

	// Connect to the shared memory object for reading, attaching anywhere.
	char* buffer = (char*) shmat(shmid, NULL, SHM_RDONLY);

	if (buffer == (void*) -1) {
		cerr << "Error attaching to shared memory object: ";
		perror(NULL);
		return 1;
	}

	// Print a nice divider to indicate success
	cerr << "=====" << endl;

	// Retrieve some info about a given shared memory buffer, then spit it out.
	struct shmid_ds shmbuffer;
	int ctlrv = shmctl(shmid, IPC_STAT, &shmbuffer); 

	if (ctlrv == -1) {
		cerr << "Error retrieving information about the shared memory segment: ";
		perror(NULL);
		return 2;
	}

	cerr << "Attached at:        " << buffer << endl;
	cerr << "Buffer Size:        " << shmbuffer.shm_segsz << endl;
	cerr << "Buffer Attach Time: " << ctime(&shmbuffer.shm_atime);
	cerr << "Buffer Change Time: " << ctime(&shmbuffer.shm_ctime);

	// Register a handler for Ctrl+C
	struct sigaction sigIntHandler;

	sigIntHandler.sa_handler = stop_running;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;

	sigaction(SIGINT, &sigIntHandler, NULL);

	// Copy over until a Ctrl+C interrupt is recieved
	while (keep_running) {
		// Do something with the frame that lives from buffer[0] to buffer[shmbuffer.shm_segsz].
		// In this case, we:

		// Convert the frame to bitpacked monochrome. (most significant bit = leftmost pixel in byte)
		// This function call has the side effect of writing the data to stdout.
		write_bits( (unsigned char*) buffer + IMG_OFFSET, shmbuffer.shm_segsz - IMG_OFFSET );

		// Don't flood our output, sleep for a period.
		// Note: usleep anyway even if the period is zero!
		// Yielding to the OS between big writes is a generally good idea.
		usleep( usleep_duration );
	}

	// Finally, detatch from the buffer.
	if (shmdt(buffer) == -1) {
		cerr << "Error detatching from shared memory object: ";
		perror(NULL);
		return 3;
	}

	return 0;
}

