# Recova

Simple Qt-based recovery tool prototype for Linux.

## Build

```
qmake recova.pro
make
```

Run the resulting `recova` binary.

The application uses `arch-chroot` and requires root privileges. It also
expects to be run within an X11 or Wayland session since it provides a Qt
graphical interface.
