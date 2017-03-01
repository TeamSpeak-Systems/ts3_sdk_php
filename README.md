TeamSpeak SDK PHP
=================
A minimalist wrapper around the TeamSpeak SDK for PHP.

Building
========
Currently the SDK can only be build for a 64bit PHP on linux with pthread support.
The build process will link against the binaries found at the location of the TeamSpeak SDK.
```
$ phpize
$ ./configure --with-teamspeak-sdk=../ts3_sdk_3.0.4
$ make
```
Testing
=======
Edit `tests/config.php` so that it points to a valid TeamSpeak-SDK-Server.
After that the tests can be run with:
```
$make test
```

Installation
============
Install the extension with:
```
$sudo make install
```
Finally, make sure to enable the just installed `ts3client` extension.

Examples
========
Can be found inside `tests/`

Documentation
=============
`ts3client.php` is a php stub that contains all methods and constants available inside the extension.
