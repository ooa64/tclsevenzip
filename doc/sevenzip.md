# sevenzip

place 7z.dll in PATH (windows) or 7z.so in LD_LIBRARY_PATH (unix)

*package require sevenzip* creates new command *sevenzip*:

	sevenzip initialize ?library?
	sevenzip isinitialized
	sevenzip extensions
	sevenzip open ?-multivolume? ?-detecttype | -forcetype type? ?-password password? ?-channel? pathOrChannel

*sevenzip open* returns archive *handle*:

	handle info
	handle count
	handle list ?-info? ?-nocase? ?-exact? ?-type f|d? ?--? ?pattern?
	handle extract ?-password password? ?-channel? itemname pathOrChannel
	handle close
