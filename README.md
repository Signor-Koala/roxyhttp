# RoxyHTTP

_A simple HTTP server written in C, and extensible through Lua_

---

## About
A (very) simple HTTP server written in C designed to simple to use and deploy in POSIX systems.

By default it simply serves files in a provided directory. This functionality can be overridden by 
functions that directly process the request and produce a response, which is done in plain Lua.

This project is currently mostly incomplete and has many rough edges.

## Prerequisites
- `cmake >= 3.22`
- `gcc >= 11.4`
- `libbsd >= 11.5`
- `lua >= 5.1`

## Setup

    git clone https://github.com/Signor-Koala/roxyhttp.git

    cd roxyhttp

    cmake -B build

    cmake --build build

The executable produced resides in `build/roxyhttp`

Currently tested with Arch Linux and Ubuntu 20.04 LTS

## Usage
### 1. Basic Setup

### Start the Server
Run the server with the following command:
```bash
./roxyhttp --lua /path/to/config.lua
```

### Static File Directory
- Create a directory named `pages` (or as defined in your `config.lua`) to store static files.
  ```bash
  mkdir pages
  ```
- Add your static files (e.g., `index.html`) into the `pages` directory.

The server will automatically serve these files when accessed via the browser.

---

### 2. Dynamic Lua Handlers

### Writing Custom Lua Handlers
Lua scripts can handle specific HTTP requests dynamically. Below is an example of a Lua handler script:

```lua
function handle_request(request)
    if request.method == "GET" then
        return 200, "Hello from Lua!"
    elseif request.method == "POST" then
        return 200, "Received POST data: " .. request.body
    else
        return 405, "Method not allowed"
    end
end
```

### Integrating Lua Handlers
1. Add the handler to your Lua configuration file:
   ```lua
   Handlers = {
       ["/dynamic"] = handle_request
   }
   ```
2. Access `http://<server_address>:<port>/dynamic` to trigger the Lua handler.

---

### 3. Command-Line Options
RoxyHTTP provides several command-line options for customization:

| Option            | Description                                 |
|-------------------|---------------------------------------------|
| `--lua`           | Path to the Lua configuration file.         |
| `--port`          | Specify a custom port (overrides `config`). |
| `--log`           | Path to the log file for server activity.   |

Example:
```bash
./roxyhttp --lua ../config.lua --port 9090
```

---

### 4. Advanced Usage

### Combining Static Files with Lua Scripting
RoxyHTTP allows combining static file serving with dynamic Lua handlers. For example:

1. Serve static files from the `pages` directory.
2. Define dynamic Lua routes for specific endpoints.
   ```lua
   Filepath = "pages"
   Handlers = {
       ["/api"] = function(request)
           return 200, "API response: " .. os.date()
       end
   }
   ```

### Caching Configuration
Customize caching options in `config.lua`:
```lua
Max_cache_entry_number = 10
Max_cache_entry_size = 8192
```

---

### 5. Troubleshooting and FAQs

### Common Issues

#### 1. **Cannot Run Config File**
**Cause**: Lua script not found or contains errors.<br/>
**Solution**:
- Ensure the file path is correct.
- Test the Lua file manually:
  ```bash
  lua /path/to/config.lua
  ```

#### 2. **Default Values Applied**
**Cause**: Configuration variables not set in `config.lua`.<br/>
**Solution**:
- Verify `config.lua` includes all necessary variables:
  ```lua
  Max_filepath = 256
  Max_response_size = 16777216
  Buffer_size = 4096
  Port = 8080
  Filepath = "pages"
  ```

#### 3. **Cannot Load Handler Table File**
**Cause**: Handlers are missing or incorrectly defined in `config.lua`.<br/>
**Solution**:
- Define handlers as shown in the [Dynamic Lua Handlers](#dynamic-lua-handlers) section.

### FAQs

#### How do I change the default port?
Edit the `Port` value in `config.lua` or use the `--port` command-line option.

#### Where are server logs stored?
Specify a log file path using the `--log` option or check `stderr` output.

---

## Examples

### Example 1: Basic Static File Serving
1. Create an `index.html` in the `pages` directory:
   ```html
   <html>
   <body>
       <h1>Welcome to RoxyHTTP</h1>
   </body>
   </html>
   ```
2. Access the server in your browser at `http://localhost:8080`.

### Example 2: Dynamic Greeting Handler
1. Add the following handler in `config.lua`:
   ```lua
   Handlers = {
       ["/greet"] = function(request)
           return 200, "Hello, Lua world!"
       end
   }
   ```
2. Access `http://localhost:8080/greet`.

---

