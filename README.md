# Proxy Server 

This repository contains a proxy server implemented in C that handles HTTP requests, reads from and writes to sockets, creates directories, checks file existence, and performs file operations like writing, reading, and printing.

## Description

The proxy server is designed to facilitate communication between clients and servers by handling HTTP requests and responses. It supports the following functionalities:

- **HTTP Request Handling**: The server parses HTTP requests from clients and processes them accordingly.
- **Socket Communication**: It establishes connections with servers using sockets to retrieve requested resources.
- **File Operations**: The server manages local files, checks for their existence, reads their contents, and writes data to them.
- **Directory Creation**: It creates directories as needed for organizing downloaded resources.

## Usage

To use the proxy server, follow these steps:

1. Compile the code using a C compiler.
2. Run the compiled executable with the appropriate command-line arguments:

   ```bash
   cproxy <URL> [-s]
   ```

   - `<URL>`: Specify the URL of the resource to retrieve.
   - `[-s]`: Optional flag to open the retrieved resource in Firefox.

3. The server will handle the HTTP request, retrieve the resource from the server, and store it locally if necessary.

## Dependencies

The proxy server code relies on standard C libraries for socket communication, file operations, and string manipulation.

## Example

Here's an example of using the proxy server:

```bash
./cproxy http://www.example.com/index.html -s
```

This command retrieves the `index.html` page from `www.example.com` and opens it in Firefox.

## Notes

- Ensure that the proxy server is compiled and executed in an environment with network access.
- Pay attention to the command-line arguments and follow the usage instructions for proper functionality.
