# sevenzip

place 7z.dll in PATH (windows) or 7z.so in LD_LIBRARY_PATH (unix)

*package require sevenzip* creates new command *sevenzip*:

	sevenzip initialize ?<library>?
	sevenzip isinitialized
	sevenzip extensions
	sevenzip formats
	sevenzip format <extension>
	sevenzip open ?-multivolume? ?-detecttype | -forcetype <type>? ?-password <password>? ?-channel? <pathOrChannel>
	sevenzip create ?-properties <propsDict>? ?-forcetype <type>? ?-password <password>? ?-channel? <pathOrChannel> <filesList>

*sevenzip open* returns archive *handle*:

	handle info
	handle count
	handle list ?-info? ?-nocase? ?-exact? ?-type f|d? ?--? ?<pattern>?
	handle extract ?-password password? ?-channel? <pathOrChannel> <itemName>
	handle close

where
	<library> - 7z.dll/7z.so
	<type> - format id (integer) or extension (string)
