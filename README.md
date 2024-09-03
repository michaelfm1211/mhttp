# mhttp

`mhttp` is a mini HTTP server. It is single threaded and is configured by
 creating a "map file". The map file contains one line for each route that will
be served. Each line is of the form `TYPE /route parameter1 parameter2 ...` and
the parameters depend on the `TYPE` of route. Here is a list of the current
supported route types:
- `FILE`: serve a static file. There are two parameters: the path the the file
to serve and the file's MIME type.
- `LINK`: serve a 302 temporary redirect. There is one parameter: the URL to
redirect to upon request.
- `TXT`: serve a short message with as `text/plain`. There is one parameter:
the text to serve.

An example map file is in the root of the project.

Usage:
```
mhttp mapfile address port
```

Example:
```
mhttp example_mapfile 127.0.0.1 8080
```
