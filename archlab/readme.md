
# Compile 

## use tk<=8.5,8.6 or above will not compile

### Modify the corresponding header file in seq/ssim.c and pipe/psim.c (assmue tk8.5 header file is located in /usr/local/tk8.5)

```c++

#ifdef HAS_GUI
#include <tk8.5/tk.h>  // #include <tk/tk.h>
#endif /* HAS_GUI */



```


### modify Makefile in sim


```mak



# Comment this out if you don't have Tcl/Tk on your system

GUIMODE=-DHAS_GUI 

# Modify the following line so that gcc can find the libtcl.so and
# libtk.so libraries on your system. You may need to use the -L option
# to tell gcc which directory to look in. Comment this out if you
# don't have Tcl/Tk.

TKLIBS=-L/usr/lib/tcl8.5/ -ltk -ltcl

# Modify the following line so that gcc can find the tcl.h and tk.h
# header files on your system. Comment this out if you don't have
# Tcl/Tk.

TKINC=-isystem /usr/include/tcl8.5/



```

### comment matherr in seq/ssim.c and pipe/psim.c

```c++

/* Hack for SunOS */
// extern int matherr();
// int *tclDummyMathPtr = (int *) matherr;



```

