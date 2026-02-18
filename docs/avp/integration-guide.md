# Integration Guide

## Overview

This guide explains how to integrate AVP-Tropic with AI agent frameworks. The TROPIC01-based hardware vault provides secure credential storage that can be accessed from any framework supporting the AVP protocol.

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         AI Agent Host System                             │
│  ┌─────────────────────────────────────────────────────────────────────┐│
│  │                    Agent Framework                                   ││
│  │  ┌─────────────────────────────────────────────────────────────────┐││
│  │  │  LangChain / CrewAI / ZeroClaw / Custom                         │││
│  │  │  Uses AVP client to request credentials                         │││
│  │  └──────────────────────────┬──────────────────────────────────────┘││
│  │                             │ AVP Protocol (JSON over transport)    ││
│  │  ┌──────────────────────────▼──────────────────────────────────────┐││
│  │  │  AVP Client (Python/TypeScript/Rust)                            │││
│  │  │  - avp.Vault("avp.toml")                                        │││
│  │  │  - vault.retrieve("anthropic_api_key")                          │││
│  │  └──────────────────────────┬──────────────────────────────────────┘││
│  └─────────────────────────────┼───────────────────────────────────────┘│
│                                │ Transport (USB Serial / Socket)        │
└────────────────────────────────┼────────────────────────────────────────┘
                                 │
┌────────────────────────────────▼────────────────────────────────────────┐
│                    AVP Hardware Device                                   │
│  ┌─────────────────────────────────────────────────────────────────────┐│
│  │  STM32U5 + AVP-Tropic Firmware                                      ││
│  │  - Receives AVP commands over USB CDC                               ││
│  │  - Translates to TROPIC01 operations                                ││
│  └──────────────────────────────┬──────────────────────────────────────┘│
│                                 │ SPI                                    │
│  ┌──────────────────────────────▼──────────────────────────────────────┐│
│  │  TROPIC01 Secure Element                                            ││
│  │  - Keys stored in tamper-resistant silicon                          ││
│  │  - Never exported                                                   ││
│  └─────────────────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────────────┘
```

## USB Transport

### Device Firmware (STM32U5 side)

The AVP-Tropic firmware exposes a USB CDC (serial) interface for communication with the host.

```c
/* USB CDC receive callback */
void CDC_Receive_Callback(uint8_t *buf, uint32_t len) {
    /* Parse AVP JSON command */
    avp_command_t cmd;
    if (parse_avp_command(buf, len, &cmd) != 0) {
        send_error_response("PARSE_ERROR");
        return;
    }

    /* Execute command */
    avp_response_t resp;
    switch (cmd.op) {
        case AVP_OP_DISCOVER:
            execute_discover(&vault, &resp);
            break;
        case AVP_OP_AUTHENTICATE:
            execute_authenticate(&vault, &cmd, &resp);
            break;
        case AVP_OP_STORE:
            execute_store(&vault, &cmd, &resp);
            break;
        case AVP_OP_RETRIEVE:
            execute_retrieve(&vault, &cmd, &resp);
            break;
        /* ... other operations ... */
    }

    /* Send JSON response */
    send_avp_response(&resp);
}
```

### Host Software (Python example)

```python
import serial
import json

class AVPSerialTransport:
    def __init__(self, port="/dev/ttyACM0", baudrate=115200):
        self.serial = serial.Serial(port, baudrate, timeout=5)

    def send_command(self, command: dict) -> dict:
        # Send JSON command
        self.serial.write(json.dumps(command).encode() + b'\n')

        # Read response
        response = self.serial.readline()
        return json.loads(response)

    def close(self):
        self.serial.close()


class AVPHardwareVault:
    def __init__(self, port="/dev/ttyACM0"):
        self.transport = AVPSerialTransport(port)
        self.session_id = None

    def discover(self):
        return self.transport.send_command({"op": "DISCOVER"})

    def authenticate(self, workspace="default", pin="123456", ttl=300):
        response = self.transport.send_command({
            "op": "AUTHENTICATE",
            "workspace": workspace,
            "auth_method": "pin",
            "auth_data": {"pin": pin},
            "requested_ttl": ttl
        })
        if response.get("ok"):
            self.session_id = response["session_id"]
        return response

    def store(self, name: str, value: str):
        return self.transport.send_command({
            "op": "STORE",
            "session_id": self.session_id,
            "name": name,
            "value": value
        })

    def retrieve(self, name: str) -> str:
        response = self.transport.send_command({
            "op": "RETRIEVE",
            "session_id": self.session_id,
            "name": name
        })
        if response.get("ok"):
            return response["value"]
        raise KeyError(f"Secret not found: {name}")

    def hw_sign(self, key_name: str, data: bytes) -> bytes:
        import base64
        response = self.transport.send_command({
            "op": "HW_SIGN",
            "session_id": self.session_id,
            "key_name": key_name,
            "data": base64.b64encode(data).decode()
        })
        if response.get("ok"):
            return base64.b64decode(response["signature"])
        raise RuntimeError(response.get("error", "Signing failed"))
```

---

## Framework Integrations

### LangChain

```python
from langchain_anthropic import ChatAnthropic
from avp_hardware import AVPHardwareVault

# Initialize hardware vault
vault = AVPHardwareVault("/dev/ttyACM0")
vault.authenticate(pin="123456")

# Get API key from hardware
api_key = vault.retrieve("anthropic_api_key")

# Use with LangChain
llm = ChatAnthropic(api_key=api_key)
response = llm.invoke("Hello, Claude!")

# Zero sensitive data
api_key = None
```

### CrewAI

```python
from crewai import Agent, Task, Crew
from avp_hardware import AVPHardwareVault

vault = AVPHardwareVault("/dev/ttyACM0")
vault.authenticate(pin="123456")

# Create agent with hardware-secured credentials
researcher = Agent(
    role="Researcher",
    goal="Research topics",
    llm_config={
        "api_key": vault.retrieve("openai_api_key")
    }
)

crew = Crew(agents=[researcher], tasks=[...])
crew.kickoff()
```

### ZeroClaw

```python
from zeroclaw import Agent
from avp_hardware import AVPHardwareVault

class AVPSecretBackend:
    """ZeroClaw SecretBackend using AVP Hardware."""

    def __init__(self, port="/dev/ttyACM0", pin="123456"):
        self.vault = AVPHardwareVault(port)
        self.vault.authenticate(pin=pin)

    def get(self, key: str) -> str:
        return self.vault.retrieve(key)

    def set(self, key: str, value: str):
        self.vault.store(key, value)

    def delete(self, key: str):
        self.vault.delete(key)

# Use with ZeroClaw
secrets = AVPSecretBackend("/dev/ttyACM0", pin="123456")
agent = Agent(secret_backend=secrets)
agent.run()
```

### Custom Agent

```python
import anthropic
from avp_hardware import AVPHardwareVault

def run_agent():
    # Initialize hardware vault
    vault = AVPHardwareVault("/dev/ttyACM0")

    # Verify device authenticity
    attestation = vault.hw_challenge()
    if not attestation["verified"]:
        raise RuntimeError("Hardware verification failed!")

    print(f"Using {attestation['model']} (verified)")

    # Authenticate
    vault.authenticate(pin="123456")

    # Get API key from hardware (never stored on disk)
    api_key = vault.retrieve("anthropic_api_key")

    # Use API
    client = anthropic.Anthropic(api_key=api_key)
    message = client.messages.create(
        model="claude-sonnet-4-20250514",
        max_tokens=1024,
        messages=[{"role": "user", "content": "Hello!"}]
    )

    # Clear sensitive data
    api_key = None

    return message.content[0].text

if __name__ == "__main__":
    print(run_agent())
```

---

## MCP Integration

AVP supports the Model Context Protocol (MCP) transport, allowing Claude and other MCP-compatible agents to directly access the hardware vault.

### MCP Server (Firmware)

The STM32U5 firmware can expose AVP operations as MCP tools:

```c
/* MCP tool definitions */
const mcp_tool_t avp_tools[] = {
    {
        .name = "avp_discover",
        .description = "Query vault capabilities",
        .input_schema = "{}",
        .handler = mcp_handle_discover
    },
    {
        .name = "avp_authenticate",
        .description = "Authenticate to vault",
        .input_schema = "{\"workspace\": \"string\", \"pin\": \"string\"}",
        .handler = mcp_handle_authenticate
    },
    {
        .name = "avp_retrieve",
        .description = "Retrieve a secret",
        .input_schema = "{\"name\": \"string\"}",
        .handler = mcp_handle_retrieve
    },
    /* ... other tools ... */
};
```

### Claude Desktop Configuration

```json
{
  "mcpServers": {
    "avp-hardware": {
      "command": "avp-mcp-bridge",
      "args": ["--port", "/dev/ttyACM0"]
    }
  }
}
```

### MCP Bridge (Host)

```python
#!/usr/bin/env python3
"""AVP MCP Bridge - Connects Claude to AVP hardware vault."""

import sys
import json
from avp_hardware import AVPHardwareVault

vault = AVPHardwareVault("/dev/ttyACM0")

def handle_tool_call(name: str, args: dict) -> dict:
    if name == "avp_discover":
        return vault.discover()
    elif name == "avp_authenticate":
        return vault.authenticate(
            args.get("workspace", "default"),
            args.get("pin", "")
        )
    elif name == "avp_retrieve":
        try:
            value = vault.retrieve(args["name"])
            return {"ok": True, "value": value}
        except KeyError:
            return {"ok": False, "error": "SECRET_NOT_FOUND"}
    # ... other operations ...

# MCP protocol loop
while True:
    line = sys.stdin.readline()
    if not line:
        break

    request = json.loads(line)
    if request.get("method") == "tools/call":
        result = handle_tool_call(
            request["params"]["name"],
            request["params"]["arguments"]
        )
        response = {
            "jsonrpc": "2.0",
            "id": request["id"],
            "result": {"content": [{"type": "text", "text": json.dumps(result)}]}
        }
        print(json.dumps(response), flush=True)
```

---

## Security Considerations

### PIN Management

```python
import getpass

# Never hardcode PINs
pin = getpass.getpass("Enter vault PIN: ")
vault.authenticate(pin=pin)

# Clear PIN from memory
pin = "0" * len(pin)
del pin
```

### Session Management

```python
# Sessions expire after TTL
vault.authenticate(pin=pin, ttl=300)  # 5 minutes

# Check session status before operations
if not vault.session_active():
    vault.authenticate(pin=pin)

# Re-authenticate on session errors
try:
    api_key = vault.retrieve("anthropic_api_key")
except SessionExpiredError:
    vault.authenticate(pin=pin)
    api_key = vault.retrieve("anthropic_api_key")
```

### Attestation

```python
# Verify device before trusting it
attestation = vault.hw_challenge()

if not attestation["verified"]:
    raise SecurityError("Device attestation failed!")

if attestation["manufacturer"] != "Tropic Square":
    raise SecurityError("Unknown device manufacturer!")

print(f"Verified: {attestation['model']} SN:{attestation['serial']}")
```

### Sensitive Data Handling

```python
import ctypes

def secure_zero(s: str) -> None:
    """Overwrite string in memory."""
    if s:
        ctypes.memset(id(s) + 48, 0, len(s))

# Use in finally block
try:
    api_key = vault.retrieve("anthropic_api_key")
    # Use api_key...
finally:
    secure_zero(api_key)
    api_key = None
```

---

## Configuration File

### avp.toml

```toml
[transport]
type = "usb"
port = "/dev/ttyACM0"  # macOS: /dev/tty.usbmodem*
baudrate = 115200

[backend]
type = "hardware"
manufacturer = "Tropic Square"
model = "TROPIC01"

[session]
default_ttl = 300
workspace = "default"

[security]
require_attestation = true
allowed_serials = ["SN123456", "SN789012"]  # Optional whitelist
```

### Loading Configuration

```python
import toml
from avp_hardware import AVPHardwareVault

config = toml.load("avp.toml")

vault = AVPHardwareVault(
    port=config["transport"]["port"],
    baudrate=config["transport"]["baudrate"]
)

# Verify device if required
if config["security"]["require_attestation"]:
    att = vault.hw_challenge()
    if not att["verified"]:
        raise RuntimeError("Attestation failed")
    if config["security"].get("allowed_serials"):
        if att["serial"] not in config["security"]["allowed_serials"]:
            raise RuntimeError(f"Unknown device: {att['serial']}")

vault.authenticate(
    workspace=config["session"]["workspace"],
    ttl=config["session"]["default_ttl"]
)
```

---

## Troubleshooting

### Device Not Found

```python
import serial.tools.list_ports

# List available ports
for port in serial.tools.list_ports.comports():
    print(f"{port.device}: {port.description}")

# Common paths:
# - Linux: /dev/ttyACM0, /dev/ttyUSB0
# - macOS: /dev/tty.usbmodem*, /dev/cu.usbmodem*
# - Windows: COM3, COM4, etc.
```

### Permission Denied (Linux)

```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER

# Or create udev rule
echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="0483", MODE="0666"' | \
    sudo tee /etc/udev/rules.d/99-avp.rules
sudo udevadm control --reload-rules
```

### Timeout Errors

```python
# Increase timeout for slow operations
vault = AVPHardwareVault(port, timeout=10)

# Or retry on timeout
import time

for attempt in range(3):
    try:
        value = vault.retrieve("key")
        break
    except TimeoutError:
        time.sleep(0.5)
```
