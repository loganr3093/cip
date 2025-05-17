# Current IP

`cip` (Current IP) is a small command-line utility made in C that displays your computer’s **local** and **public** IP addresses (both **IPv4** and **IPv6**) with color-coded output.

---

## Installation

### Requirements

- `gcc`
- `libcurl`

### Build & Install

```bash
# Clone and enter the directory
git clone https://github.com/loganr3093/cip.git
cd cip

# Build the binary
make

# Install it globally (this moves the binary to /usr/local/bin/` )
sudo make install

```

## Usage

Display your computer's IP addresses (local and/or public).

```
cip [OPTIONS]
```

### Options
```
-h, --help                Show this help message and exit
-l, --local               Show only local IP addresses
-p, --public              Show only public IP addresses
-4                        Show only IPv4 addresses
-6                        Show only IPv6 addresses
-i, --interface <name>    Show IPs for a specific network interface (e.g. eth0, wlan0)
```

### Examples
```
# Show all local and public IPv4 and IPv6 addresses
cip

# Show only local IPv4 addresses
cip -l -4

# Show only public IPv6 address
cip -p -6

# Show only IPs associated with 'eno1'
cip -i eno1
```

By default, all IP types (local & public, IPv4 & IPv6) are shown.