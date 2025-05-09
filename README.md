# cprimim

This library and cli is a tool to approximate input images with primitives. It is heavily inspired by [primitive](https://github.com/fogleman/primitive), but an independent implementation with different (probably significantly less) features and hopefully better performance.

Third party dependencies include [stb](https://github.com/nothings/stb) for image I/O and [flag.h](https://github.com/tsoding/flag.h).

For drawing shapes I used the algorithms described in [here](http://members.chello.at/easyfilter/bresenham.html), in particular the thick line algorithm but modified them to not apply antialiasing.
