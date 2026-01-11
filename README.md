# d2rreg

This is a companion CLI tool for [sh4nks/d2rloader](http://github.com/sh4nks/d2rloader) when using the Windows version of the loader.

This will allow the loader to set the registry token from within *Wine* (it will be called with ``wine d2rreg.exe --update-registry="token"``).

The reasoning behind this is that there is no way to protect the authenticator tokens using ``CryptProtectData`` natively from Linux. However, it works when Wine calls it and updates the registry with the protected token.

This tool makes it possible for the [d2rloader](http://github.com/sh4nks/d2rloader) to support the authentication method "Token"!


This is the first time me writing C++ - Please be kind :D


# License

MIT License
