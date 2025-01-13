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
./roxyhttp
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
function dynamic_handler(request)
    if request.method == "GET" then
        return {
            status = 200,
            body = "Hello from the dynamic Lua handler!",
            headers = { ["Content-Type"] = "text/plain" }
        }
    elseif request.method == "POST" then
        return {
            status = 200,
            body = "Received POST data: " .. (request.body or ""),
            headers = { ["Content-Type"] = "text/plain" }
        }
    else
        return {
            status = 405,
            body = "Method not allowed",
            headers = { ["Content-Type"] = "text/plain" }
        }
    end
end

-- Directly returning an HTML page as a string (for simple use cases)
function html_handler(request)
    return [[
        <html>
        <head><title>Simple HTML Response</title></head>
        <body>
            <h1>Welcome to the Lua-powered C Server</h1>
            <p>This is a simple HTML response directly returned as a string.</p>
        </body>
        </html>
    ]]
end
```

### Integrating Lua Handlers
#### 1. Define the Handler Table:
In the Lua handler table file (e.g., handler_table.lua), register handlers correctly. Use function names as strings as the values for the corresponding path patterns in the `Handlers` table:

   ```lua
   Handlers = {
    ["^/dynamic"] = "dynamic_handler"
}
   ```
#### 2. Avoid Direct Function Assignment:
Assigning the function directly (e.g., "/dynamic" = dynamic_handler) can lead to issues due to the way Lua scripts are executed. By using the function name as a string, the server can load the function dynamically and correctly resolve dependencies.

#### 3. Use Regex in the URI:
The URI key in the Handlers table is treated as a regex pattern. For example:

"^/dynamic$": Matches exactly /dynamic.

"^/dynamic/.*$": Matches /dynamic/anything.

Ensure the patterns align with your expected request URIs.

### Loading Lua Handler Table

#### File Separation

- The Lua handler table file is separate from the main config file (`config.lua`).
- Specify the path to the handler table in `config.lua`.

#### Accessing the Handler

Once everything is set up, you can access the handler by sending a request to the URI curl http://127.0.0.1:8080/dynamic
```
POST: curl -X POST -d "data=example" http://127.0.0.1:8080/dynamic
```
---

### 3. Advanced Usage

### Combining Static Files with Lua Scripting
RoxyHTTP allows combining static file serving with dynamic Lua handlers. For example:

1. Serve static files from the `pages` directory.
2. Define dynamic Lua routes for specific endpoints.
   ```lua
   Handlers = {
       ["^/api$"] = "api_handler"
   }

   function api_handler(request)
       return {
           status = 200,
           headers = { ["Content-Type"] = "text/plain" }
           body = "API response: " .. os.date()
       }
    end
   
   ```

### Caching Configuration
Customize caching options in `config.lua`:
```lua
Max_cache_entry_number = 10 -- Number of pages to be cached
Max_cache_entry_size = 8192 -- Max size of a page that can be cached in bytes
```
---

### 4. Troubleshooting and FAQs

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
**Cause**: Handlers are missing or incorrectly defined in `handler.lua`.<br/>
**Solution**:
- Define handlers as shown in the [Dynamic Lua Handlers](#dynamic-lua-handlers) section.
- Ensure handlers.lua and any other files it loads are correctly defined and accessible.


### FAQs

#### How do I change the default port?
Edit the `Port` value in `config.lua`.

#### Where are server logs stored?
Check `stderr` output for the server logs.

---

## Examples

### Basic Static File Serving
1. Create an `index.html` in the `pages` directory:
   ```html
   <html>
   <body>
       <h1>Welcome to RoxyHTTP</h1>
   </body>
   </html>
   ```
2. Access the server in your browser at `http://localhost:8080`.



