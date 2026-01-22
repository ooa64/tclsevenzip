# tclsevenzip

Tcl binding to the 7zip dynamic library

## usage

place 7z.dll in PATH (windows) or 7z.so in LD_LIBRARY_PATH (unix) or DYLD_LIBRARY_PATH (macos)

*package require sevenzip* creates new command *sevenzip*:

	sevenzip initialize ?<library>?
	sevenzip isinitialized
	sevenzip extensions
	sevenzip formats
	sevenzip format <extension>
	sevenzip open ?-detecttype | -forcetype <type>? ?-password <password>? ?-channel? <pathOrChannel>
	sevenzip create ?-forcetype <type>? ?-password <password>? ?-properties <propsDict>? ?-inputchannel <channel>? ?-channel? <pathOrChannel> <filesList>

*sevenzip open* returns archive *handle*:

	handle info
	handle count
	handle list ?-info? ?-nocase? ?-exact? ?-type f|d? ?--? ?<itemPattern>?
	handle extract ?-password password? ?-channel? <pathOrChannel> <itemName>
	handle close

where

	<library> - 7z.dll/7z.so
	<type> - format id (integer) or extension (string)

## [documentation](doc/sevenzip.md)
