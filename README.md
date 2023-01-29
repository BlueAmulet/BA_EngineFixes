# Blue's Oblivion Engine Fixes

Currently just one bug fix.

Oblivion looks for an underscore in a texture path with [strrchr](https://en.cppreference.com/w/c/string/byte/strrchr), but rather checking for NULL, it subtracts the result with the original string and checks if the length is negative. This happens to fail if the string is in the upper 2GB of memory.
