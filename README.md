# Recova

Simple Qt-based recovery tool prototype for Linux.
It detects mounted Linux systems that can be chrooted into and provides
common recovery actions.

## Build

```
qmake recova.pro
make
```

Run the resulting `recova` binary.

When launched, the first page attempts to chroot into each mounted
filesystem and prints the results in the integrated terminal. Successful
targets are listed for selection. After selecting a target you can run
actions such as updating packages, reinstalling GRUB, installing the LTS
kernel or creating a new administrator account.

The application uses `arch-chroot` and requires root privileges. It also
expects to be run within an X11 or Wayland session since it provides a Qt
graphical interface.
