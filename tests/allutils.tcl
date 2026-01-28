if {[lsearch [namespace children] ::tcltest] < 0} {
    package require tcltest 2
    namespace import ::tcltest::*
}

if {[info proc readFile] eq ""} {
    proc readFile {filename} {
        set f [open $filename "r"]
        try {
            fconfigure $f -encoding utf-8
            return [read $f]
        } finally {
            close $f
        }
    }
}

if {[info proc writeFile] eq ""} {
    proc writeFile {filename data} {
        set f [open $filename "w"]
        try {
            fconfigure $f -encoding utf-8
            puts -nonewline $f $data
        } finally {
            close $f
        }
    }
}

if {[info proc deleteFile] eq ""} {
    proc deleteFile {filename} {
        catch {file attributes $filename -readonly 0}
        catch {file delete -force $filename}
    }
}

if {[info exists ::tcltest::_leaks]} {
    array set ::tcltest::_leaks {count 0 bytes 0}
} elseif {[testConstraint leakTest]} {
    if {[info commands memory] eq ""} {
        puts stderr "WARNING: leakTest requested but 'memory' command not available"
    } else {
        puts stderr "WARNING: leakTest initialized - memory leaks will be reported"
        array set ::tcltest::_leaks {count 0 bytes 0}
        proc ::tcltest::_leaks_getBytes {} {
            lindex [split [memory info] \n] 3 3
        }
        proc ::tcltest::_leaks_leakTest {script {iterations 3}} {
            set end [::tcltest::_leaks_getBytes]
            for {set i 0} {$i < $iterations} {incr i} {
                uplevel 1 $script
                set tmp $end
                set end [::tcltest::_leaks_getBytes]
            }
            expr {$end - $tmp}
        }
        proc test {args} {
            set leaked [::tcltest::_leaks_leakTest {uplevel 1 [list ::tcltest::test {*}$args]} 3]
            if {$leaked != 0} {
                puts stderr "[lindex $args 0] memory leak detected: $leaked bytes"
                incr ::tcltest::_leaks(count)
                incr ::tcltest::_leaks(bytes) $leaked
            }
        }
        proc cleanupTests {args} {
            if {$::tcltest::_leaks(count) != 0} {
                # memory active [string cat [file tail [info script]] .leaks.lst]
                puts stderr "WARNING: $::tcltest::_leaks(count) tests with memory leak detected,\
                        $::tcltest::_leaks(bytes) bytes leaked"
                array set ::tcltest::_leaks {count 0 bytes 0}
            }
            uplevel 1 [list ::tcltest::cleanupTests {*}$args]
        }
    }
}
