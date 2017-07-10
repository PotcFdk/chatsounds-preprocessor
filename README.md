```
        ______            ,                            _   __
       / ___  |          /|                           | | /  \\
      / /   | |__   __ _| |_ ___  ___  _   _ _ __   __| |/ /\ \\
     / /    | '_ \ / _` | __/ __|/ _ \| | | | `_ \ / _` |\ \\\ \\
    ( (     | | | | (_| | |_\__ \ (_) | |_| | | | | (_| | \ \\\//
     \ \    |_| |_|\__,_|\______/\___/ \__,_|_| |_|\__,/ \ \ \\
      \ \____________                                   \ \/ //
       \____________ \'"`-._,-'"`-._,-'"`-._,-'"`-._,-'"`\__//
         ____  | |__) ) __ ___ _ __  _ __ ___   ___ ___  ___ ___  ___  _ __
        (____) |  ___/ '__/ _ \ '_ \| '__/ _ \ / __/ _ \/ __/ __|/ _ \| '__|
               | |   | | |  __/ |_) | | | (_) | (_(  __/\__ \__ \ (_) | |
         _____/ /    | |  \___| .__/|_|  \___/ \___\________/___/\___/| |
        (______/     | |      | |                            ^potcfdk | |
                      \|      |_|                                     |/
```

This is probably the ugliest C++ code I've ever written.
It generates lists for a specific Garry's Mod addon.
That's about it.

If you're lucky, you might get it to compile.
You need to link some boost libraries and libav.

On most modern GNU/Linux systems, install boost and libav (can be called ffmpeg-devel depending on the distribution).
Then create a build directory, `cd` into it and run `cmake ..`.

On Windows, MinGW-w64 on MSYS2 is supposed to get this compiled correctly. See `INSTALL.Win32` for further instructions.

Feel free to [create a GitHub issue](https://github.com/PotcFdk/chatsounds-preprocessor/issues) if it doesn't compile.
