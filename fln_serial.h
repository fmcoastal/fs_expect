#ifndef _fln_serial_h
#define _fin_serial_h

  
/* The values for speed are B115200, B230400, B9600, B19200, 
 * B38400, B57600, B1200, B2400, B4800, etc. The values for 
 * parity are 0 (meaning no parity), PARENB|PARODD (enable 
 * parity and use odd), PARENB (enable parity and use even), 
 * PARENB|PARODD|CMSPAR (mark parity), and PARENB|CMSPAR 
 * (space parity).
 *
 * "Blocking" sets whether a read() on the port waits for the 
 * specified number of characters to arrive. Setting no blocking 
 * means that a read() returns however many characters are 
 * available without waiting for more, up to the buffer limit.
 */

#define error_message printf


// set the Parameters fo the Serial Port
// in  fd - fopen handle
// in  speed
// in  parity
extern int set_interface_attribs (int fd, int speed, int parity);


// Determines if a serial port read should block or not
// in fd - fopen handle
extern void set_blocking (int fd, int should_block);



//#define SERIAL_EXAMPLE
#ifdef SERIAL_EXAMPLE

extern pthread_t g_tid[];

// http://www.thegeekstuff.com/2012/04/create-threads-in-linux/ 
// void * struct passed as part of the Fork Worker thread
typedef struct 
{
   int done;
   int fd;
}datablock;

// TX Thread
int tx(void* arg);

// RX Thread
extern int rx(void* arg);


// POSIX THread call to fork threads based on ThreadID
extern void *WorkerThread(void *arg);

#endif
#endif
