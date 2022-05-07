# PPM to Braille Art Converter

Uses some code from my [image generator](https://github.com/zachs18/imagegen).

Usage:

* `brailleart`: No arguments: default, black background, white foreground, 0.5 threshold
* `brailleart invert`: Single `invert` argument: white background, black foreground, 0.5 threshold
* `brailleart 0.3`: Single double argument: black background, white foreground, given threshold
* `brailleart invert 0.3`: Two arguments (first `invert`, second double): white background, black foreground, given threshold
* No other argument formats are supported at this time.
