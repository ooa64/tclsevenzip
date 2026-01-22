# tclsevenzip - Tcl binding to 7-Zip library

## Overview

The **sevenzip** package provides a Tcl interface to the 7-Zip dynamic library, enabling Tcl scripts to read and create archives in various formats including 7z, zip, tar, rar, arj, and many others.

## Installation

Before using the package, ensure the 7-Zip dynamic library is accessible:
- **Windows**: Place `7z.dll` in your PATH
- **Unix/Linux**: Place `7z.so` in your LD_LIBRARY_PATH
- **MacOS**: Place `7z.so` in your DYLD_LIBRARY_PATH

## Loading the Package

```tcl
package require sevenzip
```

This creates the `sevenzip` command with various subcommands.

## Initialization

### sevenzip initialize ?library?

Initialize the sevenzip library with an optional path to the 7-Zip dynamic library.

**Syntax:**
```tcl
sevenzip initialize ?library?
```

**Parameters:**
- `library` - (Optional) Full path to 7z.dll (Windows) or 7z.so (Unix/Linux)

**Examples:**
```tcl
# Auto-detect library from PATH/LD_LIBRARY_PATH/DYLD_LIBRARY_PATH
sevenzip initialize

# Explicit library path (Windows)
sevenzip initialize "C:/Program Files/7-Zip/7z.dll"

# Explicit library path (Unix)
sevenzip initialize "/usr/lib/p7zip/7z.so"
```

### sevenzip isinitialized

Check if the 7-Zip library has been successfully initialized.

**Returns:** `1` if initialized, `0` otherwise

**Example:**
```tcl
if {[sevenzip isinitialized]} {
    puts "7-Zip library is ready"
}
```

## Supported Formats

### sevenzip extensions

Get a list of all supported archive file extensions.

**Returns:** List of extensions (e.g., `7z zip tar rar gz bz2 xz arj cab ...`)

**Example:**
```tcl
set exts [sevenzip extensions]
puts "Supported extensions: $exts"
```

### sevenzip formats

Get detailed information about all supported archive formats.

**Returns:** List of dictionaries, each containing:
- `id` - Numeric format identifier
- `name` - Format name
- `updatable` - Whether the format supports updates (0 or 1)
- `extensions` - List of file extensions for this format

**Example:**
```tcl
foreach fmt [sevenzip formats] {
    array set f $fmt
    puts "$f(name): id=$f(id), updatable=$f(updatable), extensions=$f(extensions)"
}
```

### sevenzip format extension

Get the numeric format ID for a specific file extension.

**Parameters:**
- `extension` - File extension (e.g., "7z", "zip", "tar")

**Returns:** Numeric format ID, or `-1` if not supported

**Note:** More than one format can be detected with a given extension, the first one found is returned.

**Example:**
```tcl
set fmt [sevenzip format 7z]
if {$fmt >= 0} {
    puts "7z format ID: $fmt"
}
```

### sevenzip updatable type

Check if a format supports creating and updating archives.

**Parameters:**
- `type` - Format ID (integer) or extension (string)

**Returns:** `1` if updatable, `0` otherwise

**Example:**
```tcl
if {[sevenzip updatable 7z]} {
    puts "Can create 7z archives"
}
```

## Creating Archives

### sevenzip create

Create a new archive from a list of files.

**Syntax:**
```tcl
sevenzip create ?options? pathOrChannel filesList
```

**Options:**
- `-forcetype type` - Archive format (ID or extension)
- `-properties dict` - Archive properties (compression level, method, etc.)
- `-password password` - Encrypt archive with password
- `-inputchannel channel` - Read file contents from channel instead of disk
- `-channel` - Treat first argument as channel name

**Parameters:**
- `pathOrChannel` - Output archive path or channel name
- `filesList` - List of directories and/or files to add to archive

**Notes:**
- The file list must contain only one item when using the `-inputchannel` option.
- The archive format is determined by the file extension unless `-forcetype type` is specified.
- Given extension may belong to more than one format, the first format found is used.
- Using the `-channel` option implies using the `-forcetype type` option.

**Examples:**
```tcl
# Create simple 7z archive
sevenzip create output.7z {file1.txt file2.txt}

# Create with password
sevenzip create -password "secret" secure.7z {data.txt}

# Create zip archive
sevenzip create output.zip {doc1.pdf doc2.pdf}

# Create zip archive without extension
sevenzip create -forcetype zip output {doc1.pdf doc2.pdf}

# Create to channel
set fd [open output.7z w]
fconfigure $fd -translation binary
sevenzip create -forcetype 7z -channel $fd {file.txt}
close $fd

# Create from channel
set fd [open doc.pdf]
fconfigure $fd -translation binary
sevenzip create -inputchannel $fd output.7z {doc.pdf}
close $fd

# Create solid archive with maximal compression level using LZMA method
sevenzip create -properties {m LZMA x 9 s true} output.7z {file1.txt file2.txt}
```

## Opening Archives

### sevenzip open

Open an existing archive for reading.

**Syntax:**
```tcl
sevenzip open ?options? pathOrChannel
```

**Options:**
- `-detecttype` - Auto-detect archive format from file signature
- `-forcetype type` - Force specific format (ID or extension)
- `-password password` - Password for encrypted archives
- `-channel` - Treat argument as channel name instead of file path

**Parameters:**
- `pathOrChannel` - Path to archive file, or channel name if `-channel` is used

**Returns:** Archive handle command

**Notes**
- Only a single-volume archive can be opened using `-channel`

**Examples:**
```tcl
# Open archive by file extension
set arc [sevenzip open test.7z]

# Open with format detection
set arc [sevenzip open -detecttype test.7z]

# Open with specific format
set arc [sevenzip open -forcetype 7z archive.dat]

# Open encrypted archive
set arc [sevenzip open -password "secret" encrypted.7z]

# Open from channel
set fd [open test.7z r]
fconfigure $fd -translation binary
set arc [sevenzip open -channel $fd]
```

## Archive Handle Commands

Once an archive is opened with `sevenzip open`, it returns a handle command with the following subcommands:

### handle info

Get archive metadata and properties.

**Returns:** Dictionary with archive information, for example:
- `headerssize` - Size of archive headers
- `method` - Compression method
- `solid` - Whether archive is solid (0 or 1)
- `codepage` - Character encoding (for tar archives)
- `characts` - Character attributes (for tar archives)

**Example:**
```tcl
set arc [sevenzip open test.7z]
array set info [$arc info]
puts "Compression method: $info(method)"
puts "Solid archive: $info(solid)"
```

### handle count

Get the number of items (files and directories) in the archive.

**Returns:** Integer count

**Example:**
```tcl
set arc [sevenzip open test.7z]
puts "Archive contains [$arc count] items"
```

### handle list

List files in the archive with optional filtering and detailed information.

**Syntax:**
```tcl
handle list ?options? ?pattern?
```

**Options:**
- `-info` - Return detailed information for each item
- `-nocase` - Case-insensitive pattern matching
- `-exact` - Exact string match instead of glob pattern
- `-type f|d` - Filter by type: `f` for files, `d` for directories
- `--` - End of options marker

**Parameters:**
- `pattern` - Optional glob pattern or exact name to match

**Returns:**
- Without `-info`: List of item names
- With `-info`: List of dictionaries with item properties

**Item properties (with -info):** for example
- `name` - Item path/name
- `size` - Uncompressed size
- `packedsize` - Compressed size
- `mtime` - Modification time (Unix timestamp)
- `attrib` - File attributes (Windows)
- `mode` - File permissions (Unix)
- `isdir` - Is directory (0 or 1)
- `user` - Owner username
- `group` - Owner group name
- `encrypted` - Is encrypted (0 or 1)

**Examples:**
```tcl
set arc [sevenzip open test.7z]

# List all items
set items [$arc list]

# List with wildcard pattern
set txtFiles [$arc list *.txt]

# List directories only
set dirs [$arc list -type d]

# Get detailed information
foreach item [$arc list -info] {
    array set i $item
    puts "$i(name): $i(size) bytes, modified: $i(mtime)"
}

# Case-insensitive search
set files [$arc list -nocase README*]

# Exact match
set file [$arc list -exact path/to/file.txt]
```

### handle extract

Extract an item from the archive.

**Syntax:**
```tcl
handle extract ?options? pathOrChannel itemName
```

**Options:**
- `-password password` - Password for encrypted item
- `-channel` - Write to channel instead of file

**Parameters:**
- `pathOrChannel` - Output file path or channel name
- `itemName` - Name of item to extract (full path within archive)

**Examples:**
```tcl
set arc [sevenzip open test.7z]

# Extract to file
$arc extract output.txt path/in/archive/file.txt

# Extract with password
$arc extract -password "secret" output.txt encrypted.dat

# Extract to channel
set fd [open output.dat w]
fconfigure $fd -translation binary
$arc extract -channel $fd path/in/archive/data.bin
close $fd

# Extract to memory channel
package require tcl::chan::memchan
set mem [::tcl::chan::memchan]
$arc extract -channel $mem file.txt
seek $mem 0
set content [read $mem]
close $mem
```

### handle close

Close the archive and release resources. After calling this, the handle command is deleted.

**Example:**
```tcl
set arc [sevenzip open test.7z]
# ... use archive ...
$arc close
```

## Complete Examples

### Example 1: List Archive Contents

```tcl
package require sevenzip
sevenzip initialize

set arc [sevenzip open -detecttype myarchive.7z]
puts "Archive contains [$arc count] items:"
foreach item [$arc list -info] {
    array set i $item
    if {$i(isdir)} {
        puts "  DIR:  $i(name)/"
    } else {
        puts "  FILE: $i(name) ($i(size) bytes)"
    }
}
$arc close
```

### Example 2: Extract All Files

```tcl
package require sevenzip
sevenzip initialize

set arc [sevenzip open archive.7z]
set outdir "./extracted"
file mkdir $outdir

foreach name [$arc list -type f] {
    set outfile [file join $outdir $name]
    file mkdir [file dirname $outfile]
    $arc extract $outfile $name
    puts "Extracted: $name"
}
$arc close
```

### Example 3: Create Archive from Directory

```tcl
package require sevenzip
sevenzip initialize

proc getFilesRecursive {dir} {
    set files {}
    foreach item [glob -nocomplain -directory $dir *] {
        if {[file isfile $item]} {
            lappend files $item
        } elseif {[file isdirectory $item]} {
            lappend files {*}[getFilesRecursive $item]
        }
    }
    return $files
}

set files [getFilesRecursive "./source"]
sevenzip create -forcetype 7z backup.7z $files
puts "Created archive with [llength $files] files"
```

### Example 4: Password-Protected Archive

```tcl
package require sevenzip
sevenzip initialize

# Create encrypted archive
set files [list secret1.txt secret2.txt]
sevenzip create -forcetype 7z -password "MySecret123" encrypted.7z $files

# Open and extract
set arc [sevenzip open -password "MySecret123" encrypted.7z]
$arc extract output.txt secret1.txt
$arc close
```

### Example 5: Working with Channels

```tcl
package require sevenzip
package require tcl::chan::memchan
sevenzip initialize

# Create archive in memory
set memChan [::tcl::chan::memchan]
sevenzip create -forcetype 7z -channel $memChan [list data.txt]

# Read archive from memory
seek $memChan 0
set arc [sevenzip open -forcetype 7z -channel $memChan]
puts "Archive in memory contains: [$arc list]"
$arc close
close $memChan
```

## VFS Integration

The package includes `vfs::sevenzip` for mounting archives as virtual filesystems:

```tcl
package require vfs::sevenzip

# Mount archive as filesystem
vfs::sevenzip::Mount archive.7z /mnt/archive

# Access files normally
set fd [open /mnt/archive/file.txt r]
set content [read $fd]
close $fd

# Unmount
vfs::sevenzip::Unmount archive.7z /mnt/archive
```

## Error Handling

All commands may throw errors. Use `catch` or `try` to handle them:

```tcl
if {[catch {
    set arc [sevenzip open missing.7z]
} err]} {
    puts "Error opening archive: $err"
}

# Or with try
try {
    set arc [sevenzip open -password "wrong" encrypted.7z]
} on error {err} {
    puts "Failed: $err"
}
```

## Common Error Messages

- `Not supported` - Format not supported or corrupt file
- `Need password` - Archive is encrypted but no password provided
- `can not find channel named "..."` - Invalid channel name
- `channel "..." wasn't opened for reading` - Channel not readable
- `couldn't open file "...": no such file or directory` - File doesn't exist

## Supported Archive Formats

The library supports numerous formats including:

**Read and Write:**
- 7z (7-Zip)
- zip (ZIP)
- tar (TAR)
- wim (Windows Imaging)
- xz (XZ)
- bzip2 (BZIP2)
- gzip (GZIP)

**Read Only:**
- rar (RAR)
- arj (ARJ)
- cab (Cabinet)
- chm (CHM)
- cpio (CPIO)
- deb (Debian)
- dmg (Apple Disk Image)
- iso (ISO)
- rpm (RPM)
- And many more...

Use `sevenzip extensions` to see all supported formats.

## Notes

- Archives are opened in read-only mode. To modify, extract and recreate.
- Multi-volume archives require the first volume to be specified.
- Channel operations require binary mode: `fconfigure $ch -translation binary`
- File paths in archives use forward slashes (/) even on Windows.
- Timestamps are Unix epoch values (seconds since 1970-01-01).

## See Also

- [7-Zip Documentation](https://www.7-zip.org/)
- Tcl VFS package for virtual filesystem integration
- Tcl channel commands for I/O operations
