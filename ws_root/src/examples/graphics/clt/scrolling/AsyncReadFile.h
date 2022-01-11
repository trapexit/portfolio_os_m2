/******************************************************************************
**
**  @(#) AsyncReadFile.h 96/08/20 1.4
**
******************************************************************************/
void AsyncReadFile(char *filename, int numbytes, void **buffer, uint32 donesignal, int *result);

/*this function opens and reads a file in a background thread. The arguments are:
  filename - the name of the file to open
  numbytes - how many bytes to read. If this is 0, the entire file will be read
  buffer - a pointer to a pointer to the buffer into which to read the file. If the pointer that
    buffer points to is NULL, a buffer will be allocated and *buffer will be set to point to it
  donesignal - a signal which the background thread will send when the work is done
  result - a pointer to an integer in which the background thread will store its result*/
