#
# using tclvfs/library/zipvfs.tcl as template
#
package provide vfs::sevenzip 0.2

package require vfs [expr {$tcl_version < 9.0 ? "1.4-1.4.99" : "1.5-"}]
package require sevenzip

namespace eval vfs::sevenzip {}

proc vfs::sevenzip::Execute {zipfile args} {
    Mount $zipfile $zipfile {*}$args
    source [file join $zipfile main.tcl]
}

proc vfs::sevenzip::Mount {zipfile local args} {
    set fd [sevenzip open {*}$args [::file normalize $zipfile]]
    vfs::filesystem mount $local [list ::vfs::sevenzip::handler $fd]
    vfs::RegisterMount $local [list ::vfs::sevenzip::Unmount $fd]
    Startup $fd
    return $fd
}

proc vfs::sevenzip::Unmount {fd local} {
    Cleanup $fd
    vfs::filesystem unmount $local
    $fd close
}

proc vfs::sevenzip::handler {zipfd cmd root relative actualpath args} {
    #::vfs::log [list $zipfd $cmd $root $relative $actualpath $args]
    if {$cmd == "matchindirectory"} {
        eval [list $cmd $zipfd $relative $actualpath] $args
    } else {
        eval [list $cmd $zipfd $relative] $args
    }
}

proc vfs::sevenzip::attributes {zipfd} { return [list "state"] }
proc vfs::sevenzip::state {zipfd args} {
    vfs::attributeCantConfigure "state" "readonly" $args
}

# If we implement the commands below, we will have a perfect
# virtual file system for zip files.

proc vfs::sevenzip::matchindirectory {zipfd path actualpath pattern type} {
    #::vfs::log [list matchindirectory $path $actualpath $pattern $type]

    # This call to zip::getdir handles empty patterns properly as asking
    # for the existence of a single file $path only
    set res [GetDir $zipfd $path $pattern]
    #::vfs::log "got $res"
    if {![string length $pattern]} {
        if {![Exists $zipfd $path]} { return {} }
        set res [list $actualpath]
        set actualpath ""
    }

    set newres [list]
    foreach p [::vfs::matchCorrectTypes $type $res $actualpath] {
        lappend newres [file join $actualpath $p]
    }
    #::vfs::log "got $newres"
    return $newres
}

proc vfs::sevenzip::stat {zipfd name} {
    #::vfs::log "stat $name"
    array set sb {dev -1 uid -1 gid -1 nlink 1}

    if {$name in {{} {.}}} {
        array set sb {type directory mtime 0 size 0 mode 0777 ino -1 depth 0 name ""}
        #::vfs::log [list res [array get sb]]
        return [array get sb]
    }

    set sblist [$zipfd list -info -exact $name]
    #::vfs::log [list sevenzip [lindex $sblist 0]]
    if {[llength $sblist] == 0} {
        if {[DirExists $zipfd $name]} {
            array set sb {type directory mtime 0 size 0 mode 0777 ino -1 depth 0}
            array set sb [list name $name]
            return [array get sb]
        }
        vfs::filesystem posixerror $::vfs::posix(ENOENT)
    }
    array set sb [lindex $sblist 0]
    foreach f {dev ino nlink uid gid atime mtime ctime} {
        if {[info exists sb($f)] && ![string is integer $sb($f)]} {
            unset sb($f)
        }
    }
    foreach f {atime mtime ctime} {
        if {[info exists sb($f)] && !([string is integer $sb($f)] && $sb($f) >= -1)} {
            unset sb($f)
        }
    }
    if {[info exists sb(mode)] && ![string is integer $sb(mode)]} {
        # FIXME: 0..65536 ?
        unset sb(mode)
    }
    if {[info exists sb(size)] && ![string is wideinteger $sb(size)]} {
        unset sb(size)
    }
    if {![info exists sb(mode)] && [info exists sb(attrib)]} {
        #define FILE_ATTRIBUTE_READONLY       0x0001
        #define FILE_ATTRIBUTE_HIDDEN         0x0002
        #define FILE_ATTRIBUTE_SYSTEM         0x0004
        #define FILE_ATTRIBUTE_DIRECTORY      0x0010
        #define FILE_ATTRIBUTE_ARCHIVE        0x0020
        #define FILE_ATTRIBUTE_DEVICE         0x0040
        #define FILE_ATTRIBUTE_NORMAL         0x0080
        #define FILE_ATTRIBUTE_TEMPORARY      0x0100
        #define FILE_ATTRIBUTE_SPARSE_FILE    0x0200
        #define FILE_ATTRIBUTE_REPARSE_POINT  0x0400
        #define FILE_ATTRIBUTE_COMPRESSED     0x0800
        #define FILE_ATTRIBUTE_OFFLINE        0x1000
        #define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED 0x2000
        #define FILE_ATTRIBUTE_ENCRYPTED      0x4000
        #define _S_IFMT   0170000 0xf000      /* file type mask */
        #define _S_IFDIR  0040000 0x4000      /* directory */
        #define _S_IFCHR  0020000 0x2000      /* character special */
        #define _S_IFIFO  0010000 0x1000      /* pipe */
        #define _S_IFREG  0100000 0x8000      /* regular */
        #define _S_IREAD  0000400 0x0100      /* read permission, owner */
        #define _S_IWRITE 0000200 0x0080      /* write permission, owner */
        #define _S_IEXEC  0000100 0x0040      /* execute/search permission, owner */
        set isdir [expr {$sb(attrib) & 0x0010}]
        set isreadonly [expr {$sb(attrib) & 0x0001}]
        set sb(mode) [expr {$isdir ? 0x4000 : 0x8000}]
        set sb(mode) [expr {$sb(mode) | ($isreadonly && !$isdir ? 0x16d : 0x1ff)}]
    }
    if {![info exists sb(mode)]} {
        set sb(mode) [expr {[info exists sb(isdir)] && $sb(isdir) ? 0x41ff : 0x816d}]
    }
    if {![info exists sb(type)] && [info exists sb(isdir)]} {
        set sb(type) [expr {$sb(isdir) ? "directory" : "file"}]
    }
    if {![info exists sb(type)]} {
        set sb(type) [expr {$sb(mode) & 0x4000 ? "directory" : "file"}]
    }
    if {![info exists sb(atime)] && [info exists sb(mtime)]} {
        set sb(atime) $sb(mtime)
    }
    if {![info exists sb(ctime)] && [info exists sb(mtime)]} {
        set sb(ctime) $sb(mtime)
    }
    #::vfs::log [list res [array get sb]]
    array get sb
}

proc vfs::sevenzip::access {zipfd name mode} {
    #::vfs::log "zip-access $name $mode"
    if {$mode & 2} {
        vfs::filesystem posixerror $::vfs::posix(EROFS)
    }
    # Readable, Exists and Executable are treated as 'exists'
    # Could we get more information from the archive?
    if {[Exists $zipfd $name]} {
        return 1
    } else {
        error "No such file"
    }

}

proc vfs::sevenzip::open {zipfd name mode permissions} {
    #::vfs::log "open $name $mode $permissions"
    # return a list of two elements:
    # 1. first element is the Tcl channel name which has been opened
    # 2. second element (optional) is a command to evaluate when
    #    the channel is closed.

    switch -- $mode {
        "" -
        "r" {
            if {[llength [$zipfd list -exact $name]] == 0} {
                vfs::filesystem posixerror $::vfs::posix(ENOENT)
            }
            if {[llength [$zipfd list -type f -exact $name]] == 0} {
                vfs::filesystem posixerror $::vfs::posix(EISDIR)
            }
            set nfd [vfs::memchan]
            fconfigure $nfd -translation binary
            $zipfd extract -c $name $nfd
            fconfigure $nfd -translation auto
            seek $nfd 0

            return [list $nfd]
        }
        default {
            vfs::filesystem posixerror $::vfs::posix(EROFS)
        }
    }
}

proc vfs::sevenzip::createdirectory {zipfd name} {
    #::vfs::log "createdirectory $name"
    vfs::filesystem posixerror $::vfs::posix(EROFS)
}

proc vfs::sevenzip::removedirectory {zipfd name recursive} {
    #::vfs::log "removedirectory $name"
    vfs::filesystem posixerror $::vfs::posix(EROFS)
}

proc vfs::sevenzip::deletefile {zipfd name} {
    #::vfs::log "deletefile $name"
    vfs::filesystem posixerror $::vfs::posix(EROFS)
}

proc vfs::sevenzip::fileattributes {zipfd name args} {
    #::vfs::log "fileattributes $args"
    switch -- [llength $args] {
        0 {
            # list strings
            return [list]
        }
        1 {
            # get value
            set index [lindex $args 0]
            return ""
        }
        2 {
            # set value
            set index [lindex $args 0]
            set val [lindex $args 1]
            vfs::filesystem posixerror $::vfs::posix(EROFS)
        }
    }
}

proc vfs::sevenzip::utime {fd path actime mtime} {
    vfs::filesystem posixerror $::vfs::posix(EROFS)
}

#
# Some archives do not have directory entries (for example, arj or some zips)
# Therefore, we need to create such entries in the Cache.
#
# Cache is a dictionary with directories as keys, files are stored in a special directory "."
# 
# sevenzipX { . { file1 file2 } dir1 { . {} subdir1 { . { file3 file4 } } subdir2 { . {} } } } 
#
# file1
# file2
# dir1
#     subdir1
#         file3
#         file4 
#     subdir2
#

interp alias {} vfs::sevenzip::Startup {} vfs::sevenzip::CacheCreate
interp alias {} vfs::sevenzip::Cleanup {} vfs::sevenzip::CacheClear
interp alias {} vfs::sevenzip::GetDir {} vfs::sevenzip::CacheGetDir
interp alias {} vfs::sevenzip::DirExists {} vfs::sevenzip::CacheDirExists

proc vfs::sevenzip::Exists {fd name} {
    expr {[llength [$fd list -exact $name]] || [CacheDirExists $fd $name]}
}

namespace eval vfs::sevenzip {
    variable Cache [dict create]
}

proc vfs::sevenzip::CacheCreate {mnt} {
    dict set ::vfs::sevenzip::Cache $mnt {. {}}
    foreach item [$mnt list -info] {
        if {[dict exist $item "path"]} {
            set parts [file split [dict get $item "path"]]
        } else {
            set parts [list {[Content]}]
        }
        set path [lrange $parts 0 end-1]
        set name [lindex $parts end]

        for {set i 0} {$i < [llength $path]} {incr i} {
            set p [lrange $path 0 $i]
            if {![dict exists $::vfs::sevenzip::Cache $mnt {*}$p]} {
                dict set ::vfs::sevenzip::Cache $mnt {*}$p [list "." {}]
            }
        }
        if {[dict get $item "isdir"]} {
            if {![dict exist $::vfs::sevenzip::Cache $mnt {*}$path $name]} {
                dict set ::vfs::sevenzip::Cache $mnt {*}$path $name [list "." {}]
            }
        } else {
            dict set ::vfs::sevenzip::Cache $mnt {*}$path "." [concat \
                    [dict get $::vfs::sevenzip::Cache $mnt {*}$path "."] [list $name]]
        }
    }
}

proc vfs::sevenzip::CacheClear {mnt} {
    dict unset ::vfs::sevenzip::Cache $mnt
}

proc vfs::sevenzip::CacheDirExists {mnt item} {
    dict exists $::vfs::sevenzip::Cache $mnt {*}[file split $item] "."
}

proc vfs::sevenzip::CacheFileExists {mnt item} {
    set parts [file split $item]
    set path [lrange $parts 0 end-1]
    set file [lindex $parts end]
    if {[dict exists $::vfs::sevenzip::Cache $mnt {*}$path "."]} {
        if {[lsearch [dict get $::vfs::sevenzip::Cache $mnt {*}$path "."] $file] >= 0} {
            return 1
        }
    }
    return 0
}

proc vfs::sevenzip::CacheGetDir {mnt path {pat *}} {
    set parts [file split $path]
    if {![dict exists $::vfs::sevenzip::Cache $mnt {*}$parts "."]} {
        return {}
    }
    if {$pat == ""} {
        return [list $path]
    }
    set dirs [dict keys [dict get $::vfs::sevenzip::Cache $mnt {*}$parts]]
    set files [dict get $::vfs::sevenzip::Cache $mnt {*}$parts "."]
    set content [concat [lsearch -all -inline -not $dirs "."] $files]

    if {$pat == "*"} {
        return $content
    } 
    set rc [list]
    foreach f $content {
        if {[string match -nocase $pat $f]} {
            lappend rc $f
        }
    }
    return $rc
}
