# librats Python Bindings

This document provides technical details about the Python bindings for librats.

## Architecture Overview

The Python bindings provide a high-level, Pythonic interface to the librats C library using ctypes:

```
┌─────────────────────────────────────────────────────────────────┐
│                        Python Application                       │
├─────────────────────────────────────────────────────────────────┤
│                      librats_py Package                        │
│  ┌─────────────┐  ┌──────────────┐  ┌─────────────────────────┐ │
│  │    core.py  │  │ exceptions.py │  │      callbacks.py       │ │
│  │             │  │              │  │                         │ │
│  │ RatsClient  │  │  RatsError   │  │ Callback Type Defs      │ │
│  └─────────────┘  └──────────────┘  └─────────────────────────┘ │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │                 ctypes_wrapper.py                          │ │
│  │           Low-level C API bindings                         │ │
│  └─────────────────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│                    librats C Library                           │
│                  (librats.so/.dll/.dylib)                      │
└─────────────────────────────────────────────────────────────────┘
```

## Module Structure

### Core Modules

- **`core.py`** - Main `RatsClient` class with high-level API
- **`ctypes_wrapper.py`** - Low-level ctypes bindings to C library
- **`exceptions.py`** - Exception classes and error handling
- **`enums.py`** - Enumerations and constants
- **`callbacks.py`** - Callback type definitions

### Supporting Files

- **`examples/`** - Example applications demonstrating usage
- **`tests/`** - Unit and integration tests
- **`setup.py`** - Package installation configuration
- **`pyproject.toml`** - Modern Python packaging configuration

## Design Principles

### 1. Pythonic Interface

The bindings provide a clean, Pythonic API that hides C-specific details:

```python
# Pythonic - context manager, exceptions, typed parameters
with RatsClient(8080) as client:
    client.start()
    client.connect("localhost", 8081)
    client.send_string(peer_id, "Hello!")

# Not this (C-style)
handle = rats_create(8080)
result = rats_start(handle)
if result != RATS_SUCCESS:
    # handle error
```

### 2. Memory Management

Python handles memory management automatically:

- C strings returned from librats are automatically freed
- Python objects manage callback lifetime
- Context managers ensure proper cleanup

### 3. Type Safety

Strong typing with proper Python types:

```python
def send_string(self, peer_id: str, message: str) -> None:
def get_peer_count(self) -> int:
def get_connection_statistics(self) -> Dict[str, Any]:
```

### 4. Error Handling

Python exceptions instead of error codes:

```python
try:
    client.connect("invalid-host", 8080)
except RatsConnectionError as e:
    print(f"Connection failed: {e}")
```

## Callback System

The callback system bridges C function pointers with Python functions:

### C Callback Types

```c
typedef void (*rats_connection_cb)(void* user_data, const char* peer_id);
typedef void (*rats_string_cb)(void* user_data, const char* peer_id, const char* message);
```

### Python Callback Wrapper

```python
def _create_string_callback(self, callback: StringCallback):
    def c_callback(user_data, peer_id_ptr, message_ptr):
        if callback and peer_id_ptr and message_ptr:
            peer_id = peer_id_ptr.decode('utf-8')
            message = message_ptr.decode('utf-8')
            try:
                callback(peer_id, message)
            except Exception as e:
                print(f"Error in string callback: {e}")
    return StringCallbackType(c_callback)
```

### Thread Safety

- Callbacks are executed in C library threads
- Python GIL protects callback execution
- Exception handling prevents crashes

## Error Handling Strategy

### Exception Hierarchy

```
RatsError (base)
├── RatsConnectionError
├── RatsInvalidParameterError  
├── RatsNotRunningError
├── RatsPeerNotFoundError
└── RatsJsonParseError
```

### Error Code Mapping

C error codes are automatically converted to appropriate Python exceptions:

```python
def check_error(error_code: int, operation: str = "Operation"):
    if error_code == ErrorCode.SUCCESS:
        return
    
    error_enum = ErrorCode(error_code)
    if error_enum == ErrorCode.INVALID_PARAMETER:
        raise RatsInvalidParameterError(f"{operation} failed")
    # ... etc
```

## Library Loading

The bindings automatically locate the librats shared library:

### Search Strategy

1. Current directory
2. Relative build directories (`../build`, `../../build`)  
3. System library paths (`/usr/lib`, `/usr/local/lib`)
4. Environment paths (`LD_LIBRARY_PATH`, `PATH`)
5. Package-bundled libraries

### Platform-Specific Names

- **Windows**: `librats.dll`, `rats.dll`
- **macOS**: `librats.dylib`, `librats.so`  
- **Linux**: `librats.so`, `librats.so.1`

## Testing Strategy

### Test Categories

1. **Unit Tests** (`test_client.py`)
   - Test individual methods and properties
   - Mock/skip tests when library unavailable
   - Focus on API correctness

2. **Integration Tests** (`test_integration.py`)
   - Test actual P2P communication
   - Multiple client scenarios
   - Real network operations

3. **Example Tests**
   - Verify examples import and run
   - Syntax and basic functionality checks

### Test Execution

```bash
# Run all tests
python -m pytest librats_py/tests/ -v

# Run only unit tests (works without native library)  
python -m pytest librats_py/tests/test_client.py -v

# Run integration tests (requires native library)
python -m pytest librats_py/tests/test_integration.py -v

# Run with coverage
python -m pytest --cov=librats_py librats_py/tests/
```

## Build Process

### Development Build

```bash
# Build native library
mkdir build && cd build
cmake ..
make

# Install Python bindings  
cd ../python
pip install -e .
```

### Using Build Script

```bash
cd python

# Full build process
python build.py --all

# Individual steps
python build.py --build-native
python build.py --install
python build.py --test
```

## Deployment Considerations

### Library Distribution

**Option 1: Separate Installation**
- User builds/installs librats C library separately
- Python package finds library at runtime
- Smaller Python package size

**Option 2: Bundled Libraries**
- Include compiled libraries in Python package
- Platform-specific wheels
- Larger package but easier installation

### Platform Support

- **Linux**: Primary development platform
- **Windows**: Supported via MSVC or MinGW builds
- **macOS**: Supported with clang/cmake

### Python Version Support

- **Minimum**: Python 3.7
- **Tested**: Python 3.7, 3.8, 3.9, 3.10, 3.11, 3.12
- **Dependencies**: Only standard library (ctypes)

## Performance Considerations

### ctypes Overhead

- Function calls have some overhead compared to native extensions
- Negligible for network operations (I/O bound)
- Critical paths still in C library

### Memory Efficiency

- Minimal Python object overhead
- C library handles heavy lifting
- String conversions only at API boundaries

### Threading

- C library manages threads internally
- Python callbacks executed with GIL
- Non-blocking operations encouraged

## Extension Points

### Adding New APIs

1. **Add C function signature** in `ctypes_wrapper.py`:
```python
self.lib.rats_new_function.argtypes = [c_void_p, c_char_p]
self.lib.rats_new_function.restype = c_int
```

2. **Add Python wrapper** in `core.py`:
```python
def new_function(self, param: str) -> None:
    param_bytes = param.encode('utf-8')
    result = self._lib.lib.rats_new_function(self._handle, param_bytes)
    check_error(result, "New function")
```

### Custom Callbacks

Add new callback types in `callbacks.py` and implement wrappers in `core.py`.

## Troubleshooting

### Common Issues

1. **Library Not Found**
   - Check build completed successfully
   - Verify library in search path
   - Check platform-specific library name

2. **Import Errors**
   - Usually indicates missing native library
   - Tests will skip automatically if library unavailable

3. **Callback Crashes**
   - Exception in callback causes print, not crash
   - Check callback signature matches expected type

4. **Memory Issues**
   - Use context managers (`with` statements)
   - Don't keep references to freed C objects

### Debug Mode

Enable verbose logging to diagnose issues:

```python
RatsClient.set_logging_enabled(True)
RatsClient.set_log_level(LogLevel.DEBUG)
```

## Future Enhancements

### Async Support

Consider adding asyncio support for non-blocking operations:

```python
async with AsyncRatsClient(8080) as client:
    await client.start()
    await client.connect("localhost", 8081)
```

### Type Stubs

Generate `.pyi` stub files for better IDE support and static analysis.

### Cython Alternative

For performance-critical applications, consider Cython-based bindings as an alternative to ctypes.

### Documentation

- Auto-generate API docs from docstrings
- Interactive examples in documentation
- Video tutorials for common use cases
