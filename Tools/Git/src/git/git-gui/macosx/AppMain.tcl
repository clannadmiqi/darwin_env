set gitprefix [regsub "/usr/.*" [file normalize [info script]] "/usr"]
set gitexecdir [file join $gitprefix libexec git-core]
set gitguilib  [file join $gitprefix share git-gui lib]
set env(PATH) "$gitexecdir:$env(PATH)"

if {[string first -psn [lindex $argv 0]] == 0} {
	lset argv 0 [file join $gitexecdir git-gui]
}

if {[file tail [lindex $argv 0]] eq {gitk}} {
	set argv0 [lindex $argv 0]
	set AppMain_source $argv0
} else {
	set argv0 [file join $gitexecdir [file tail [lindex $argv 0]]]
	set AppMain_source [file join $gitguilib git-gui.tcl]
	if {[pwd] eq {/}} {
		cd $env(HOME)
	}
}

unset gitexecdir gitguilib
set argv [lrange $argv 1 end]
source $AppMain_source
