**SimpleFileServer**

A non-blocking epoll-event-driven File Server that distributes files to anyone making a request. Uppon startup
a base directory is passed as the absolute path

Each request has the format: "/path/to/file\r\n\r\”" where the path to file is from the base directory passed uppon startup.
Any paths ending or beginning with a "/" or containing ".." are not allowed to avoid directory traversal.

Compile it by entering the build directory, then type

```
cmake ..
```

Followed by

```
make
```

to start the server, then type

```
./SimpleFileServer <port> <base_directory> <optional: number of worker threads>
```

Note that, due to simplicity reasons, this is not at all safe for use in any relevant environment. The server is yet unable to discard invalid request formats and there is no guarantee that the path passed by the request is safe in every way. It is just meant as a demonstration and practice for epoll-based event-driven IO.
