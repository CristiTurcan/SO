# Operating Systems Course Project

  

This projects generates statistics files about regular files, symbolic links and bmp images. The statistics are about name, last modified date, user access rights.

  

The images are converted to black & white using the file's format.

  

The project's scope was to use processes for every different functionality and use pipes for their communication. Also, the projects uses a shell script for counter a char in the text files.

  

# Source Files

  

* `proiect.c`: C source file
* `charCounter.sh`: Counts character occurences in correct senteces.

  

# Run

`gcc -Wall -o proiect proiect.c`
`./proiect.c <input_director>  <output_director>  <charcater>`