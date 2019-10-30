# git-gui index (add/remove) support
# Copyright (C) 2006, 2007 Shawn Pearce

proc _delete_indexlock {} {
	if {[catch {file delete -- [gitdir index.lock]} err]} {
		error_popup [strcat [mc "Unable to unlock the index."] "\n\n$err"]
	}
}

proc _close_updateindex {fd after} {
	global use_ttk NS
	fconfigure $fd -blocking 1
	if {[catch {close $fd} err]} {
		set w .indexfried
		Dialog $w
		wm withdraw $w
		wm title $w [strcat "[appname] ([reponame]): " [mc "Index Error"]]
		wm geometry $w "+[winfo rootx .]+[winfo rooty .]"
		set s [mc "Updating the Git index failed.  A rescan will be automatically started to resynchronize git-gui."]
		text $w.msg -yscrollcommand [list $w.vs set] \
			-width [string length $s] -relief flat \
			-borderwidth 0 -highlightthickness 0 \
			-background [get_bg_color $w]
		$w.msg tag configure bold -font font_uibold -justify center
		${NS}::scrollbar $w.vs -command [list $w.msg yview]
		$w.msg insert end $s bold \n\n$err {}
		$w.msg configure -state disabled

		${NS}::button $w.continue \
			-text [mc "Continue"] \
			-command [list destroy $w]
		${NS}::button $w.unlock \
			-text [mc "Unlock Index"] \
			-command "destroy $w; _delete_indexlock"
		grid $w.msg - $w.vs -sticky news
		grid $w.unlock $w.continue - -sticky se -padx 2 -pady 2
		grid columnconfigure $w 0 -weight 1
		grid rowconfigure $w 0 -weight 1

		wm protocol $w WM_DELETE_WINDOW update
		bind $w.continue <Visibility> "
			grab $w
			focus %W
		"
		wm deiconify $w
		tkwait window $w

		$::main_status stop
		unlock_index
		rescan $after 0
		return
	}

	$::main_status stop
	unlock_index
	uplevel #0 $after
}

proc update_indexinfo {msg path_list after} {
	global update_index_cp

	if {![lock_index update]} return

	set update_index_cp 0
	set path_list [lsort $path_list]
	set total_cnt [llength $path_list]
	set batch [expr {int($total_cnt * .01) + 1}]
	if {$batch > 25} {set batch 25}

	$::main_status start $msg [mc "files"]
	set fd [git_write update-index -z --index-info]
	fconfigure $fd \
		-blocking 0 \
		-buffering full \
		-buffersize 512 \
		-encoding binary \
		-translation binary
	fileevent $fd writable [list \
		write_update_indexinfo \
		$fd \
		$path_list \
		$total_cnt \
		$batch \
		$after \
		]
}

proc write_update_indexinfo {fd path_list total_cnt batch after} {
	global update_index_cp
	global file_states current_diff_path

	if {$update_index_cp >= $total_cnt} {
		_close_updateindex $fd $after
		return
	}

	for {set i $batch} \
		{$update_index_cp < $total_cnt && $i > 0} \
		{incr i -1} {
		set path [lindex $path_list $update_index_cp]
		incr update_index_cp

		set s $file_states($path)
		switch -glob -- [lindex $s 0] {
		A? {set new _O}
		MT -
		TM -
		T_ {set new _T}
		M? {set new _M}
		TD -
		D_ {set new _D}
		D? {set new _?}
		?? {continue}
		}
		set info [lindex $s 2]
		if {$info eq {}} continue

		puts -nonewline $fd "$info\t[encoding convertto utf-8 $path]\0"
		display_file $path $new
	}

	$::main_status update $update_index_cp $total_cnt
}

proc update_index {msg path_list after} {
	global update_index_cp

	if {![lock_index update]} return

	set update_index_cp 0
	set path_list [lsort $path_list]
	set total_cnt [llength $path_list]
	set batch [expr {int($total_cnt * .01) + 1}]
	if {$batch > 25} {set batch 25}

	$::main_status start $msg [mc "files"]
	set fd [git_write update-index --add --remove -z --stdin]
	fconfigure $fd \
		-blocking 0 \
		-buffering full \
		-buffersize 512 \
		-encoding binary \
		-translation binary
	fileevent $fd writable [list \
		write_update_index \
		$fd \
		$path_list \
		$total_cnt \
		$batch \
		$after \
		]
}

proc write_update_index {fd path_list total_cnt batch after} {
	global update_index_cp
	global file_states current_diff_path

	if {$update_index_cp >= $total_cnt} {
		_close_updateindex $fd $after
		return
	}

	for {set i $batch} \
		{$update_index_cp < $total_cnt && $i > 0} \
		{incr i -1} {
		set path [lindex $path_list $update_index_cp]
		incr update_index_cp

		switch -glob -- [lindex $file_states($path) 0] {
		AD {set new __}
		?D {set new D_}
		_O -
		AT -
		AM {set new A_}
		TM -
		MT -
		_T {set new T_}
		_U -
		U? {
			if {[file exists $path]} {
				set new M_
			} else {
				set new D_
			}
		}
		?M {set new M_}
		?? {continue}
		}
		puts -nonewline $fd "[encoding convertto utf-8 $path]\0"
		display_file $path $new
	}

	$::main_status update $update_index_cp $total_cnt
}

proc checkout_index {msg path_list after} {
	global update_index_cp

	if {![lock_index update]} return

	set update_index_cp 0
	set path_list [lsort $path_list]
	set total_cnt [llength $path_list]
	set batch [expr {int($total_cnt * .01) + 1}]
	if {$batch > 25} {set batch 25}

	$::main_status start $msg [mc "files"]
	set fd [git_write checkout-index \
		--index \
		--quiet \
		--force \
		-z \
		--stdin \
		]
	fconfigure $fd \
		-blocking 0 \
		-buffering full \
		-buffersize 512 \
		-encoding binary \
		-translation binary
	fileevent $fd writable [list \
		write_checkout_index \
		$fd \
		$path_list \
		$total_cnt \
		$batch \
		$after \
		]
}

proc write_checkout_index {fd path_list total_cnt batch after} {
	global update_index_cp
	global file_states current_diff_path

	if {$update_index_cp >= $total_cnt} {
		_close_updateindex $fd $after
		return
	}

	for {set i $batch} \
		{$update_index_cp < $total_cnt && $i > 0} \
		{incr i -1} {
		set path [lindex $path_list $update_index_cp]
		incr update_index_cp
		switch -glob -- [lindex $file_states($path) 0] {
		U? {continue}
		?M -
		?T -
		?D {
			puts -nonewline $fd "[encoding convertto utf-8 $path]\0"
			display_file $path ?_
		}
		}
	}

	$::main_status update $update_index_cp $total_cnt
}

proc unstage_helper {txt paths} {
	global file_states current_diff_path

	if {![lock_index begin-update]} return

	set path_list [list]
	set after {}
	foreach path $paths {
		switch -glob -- [lindex $file_states($path) 0] {
		A? -
		M? -
		T? -
		D? {
			lappend path_list $path
			if {$path eq $current_diff_path} {
				set after {reshow_diff;}
			}
		}
		}
	}
	if {$path_list eq {}} {
		unlock_index
	} else {
		update_indexinfo \
			$txt \
			$path_list \
			[concat $after [list ui_ready]]
	}
}

proc do_unstage_selection {} {
	global current_diff_path selected_paths

	if {[array size selected_paths] > 0} {
		unstage_helper \
			[mc "Unstaging selected files from commit"] \
			[array names selected_paths]
	} elseif {$current_diff_path ne {}} {
		unstage_helper \
			[mc "Unstaging %s from commit" [short_path $current_diff_path]] \
			[list $current_diff_path]
	}
}

proc add_helper {txt paths} {
	global file_states current_diff_path

	if {![lock_index begin-update]} return

	set path_list [list]
	set after {}
	foreach path $paths {
		switch -glob -- [lindex $file_states($path) 0] {
		_U -
		U? {
			if {$path eq $current_diff_path} {
				unlock_index
				merge_stage_workdir $path
				return
			}
		}
		_O -
		?M -
		?D -
		?T {
			lappend path_list $path
			if {$path eq $current_diff_path} {
				set after {reshow_diff;}
			}
		}
		}
	}
	if {$path_list eq {}} {
		unlock_index
	} else {
		update_index \
			$txt \
			$path_list \
			[concat $after {ui_status [mc "Ready to commit."]}]
	}
}

proc do_add_selection {} {
	global current_diff_path selected_paths

	if {[array size selected_paths] > 0} {
		add_helper \
			[mc "Adding selected files"] \
			[array names selected_paths]
	} elseif {$current_diff_path ne {}} {
		add_helper \
			[mc "Adding %s" [short_path $current_diff_path]] \
			[list $current_diff_path]
	}
}

proc do_add_all {} {
	global file_states

	set paths [list]
	set untracked_paths [list]
	foreach path [array names file_states] {
		switch -glob -- [lindex $file_states($path) 0] {
		U? {continue}
		?M -
		?T -
		?D {lappend paths $path}
		?O {lappend untracked_paths $path}
		}
	}
	if {[llength $untracked_paths]} {
		set reply 0
		switch -- [get_config gui.stageuntracked] {
		no {
			set reply 0
		}
		yes {
			set reply 1
		}
		ask -
		default {
			set reply [ask_popup [mc "Stage %d untracked files?" \
									  [llength $untracked_paths]]]
		}
		}
		if {$reply} {
			set paths [concat $paths $untracked_paths]
		}
	}
	add_helper [mc "Adding all changed files"] $paths
}

proc revert_helper {txt paths} {
	global file_states current_diff_path

	if {![lock_index begin-update]} return

	# The index is now locked. Some of the paths below include calls that
	# unlock the index (e.g. checked_index). If we reach the end and the
	# index is still locked, we need to unlock it before returning.
	set need_unlock_index 1

	set path_list [list]
	set untracked_list [list]
	set after {}
	foreach path $paths {
		switch -glob -- [lindex $file_states($path) 0] {
		U? {continue}
		?O {
			lappend untracked_list $path
		}
		?M -
		?T -
		?D {
			lappend path_list $path
			if {$path eq $current_diff_path} {
				set after {reshow_diff;}
			}
		}
		}
	}

	set path_cnt [llength $path_list]
	set untracked_cnt [llength $untracked_list]

	if {$path_cnt > 0} {
		# Split question between singular and plural cases, because
		# such distinction is needed in some languages. Previously, the
		# code used "Revert changes in" for both, but that can't work
		# in languages where 'in' must be combined with word from
		# rest of string (in different way for both cases of course).
		#
		# FIXME: Unfortunately, even that isn't enough in some languages
		# as they have quite complex plural-form rules. Unfortunately,
		# msgcat doesn't seem to support that kind of string
		# translation.
		#
		if {$path_cnt == 1} {
			set query [mc \
				"Revert changes in file %s?" \
				[short_path [lindex $path_list]] \
				]
		} else {
			set query [mc \
				"Revert changes in these %i files?" \
				$path_cnt]
		}

		set reply [tk_dialog \
			.confirm_revert \
			"[appname] ([reponame])" \
			"$query

[mc "Any unstaged changes will be permanently lost by the revert."]" \
			question \
			1 \
			[mc "Do Nothing"] \
			[mc "Revert Changes"] \
			]

		if {$reply == 1} {
			checkout_index \
				$txt \
				$path_list \
				[concat $after [list ui_ready]]

			set need_unlock_index 0
		}
	}

	if {$need_unlock_index} { unlock_index }

	if {$untracked_cnt > 0} {
		# Split question between singular and plural cases, because
		# such distinction is needed in some languages.
		#
		# FIXME: Unfortunately, even that isn't enough in some languages
		# as they have quite complex plural-form rules. Unfortunately,
		# msgcat doesn't seem to support that kind of string
		# translation.
		#
		if {$untracked_cnt == 1} {
			set query [mc \
				"Delete untracked file %s?" \
				[short_path [lindex $untracked_list]] \
				]
		} else {
			set query [mc \
				"Delete these %i untracked files?" \
				$untracked_cnt \
				]
		}

		set reply [tk_dialog \
			.confirm_revert \
			"[appname] ([reponame])" \
			"$query

[mc "Files will be permanently deleted."]" \
			question \
			1 \
			[mc "Do Nothing"] \
			[mc "Delete Files"] \
			]

		if {$reply == 1} {
			delete_files $untracked_list
		}
	}
}

# Delete all of the specified files, performing deletion in batches to allow the
# UI to remain responsive and updated.
proc delete_files {path_list} {
	# Enable progress bar status updates
	$::main_status start [mc "Deleting"] [mc "files"]

	set path_index 0
	set deletion_errors [list]
	set deletion_error_path "not yet captured"
	set batch_size 50

	delete_helper \
		$path_list \
		$path_index \
		$deletion_errors \
		$deletion_error_path \
		$batch_size
}

# Helper function to delete a list of files in batches. Each call deletes one
# batch of files, and then schedules a call for the next batch after any UI
# messages have been processed.
proc delete_helper \
	{path_list path_index deletion_errors deletion_error_path batch_size} {
	global file_states

	set path_cnt [llength $path_list]

	set batch_remaining $batch_size

	while {$batch_remaining > 0} {
		if {$path_index >= $path_cnt} { break }

		set path [lindex $path_list $path_index]

		set deletion_failed [catch {file delete -- $path} deletion_error]

		if {$deletion_failed} {
			lappend deletion_errors $deletion_error

			# Optimistically capture the path that failed, in case
			# there's only one.
			set deletion_error_path $path
		} else {
			remove_empty_directories [file dirname $path]

			# Don't assume the deletion worked. Remove the file from
			# the UI, but only if it no longer exists.
			if {![lexists $path]} {
				unset file_states($path)
				display_file $path __
			}
		}

		incr path_index 1
		incr batch_remaining -1
	}

	# Update the progress bar to indicate that this batch has been
	# completed. The update will be visible when this procedure returns
	# and allows the UI thread to process messages.
	$::main_status update $path_index $path_cnt

	if {$path_index < $path_cnt} {
		# The Tcler's Wiki lists this as the best practice for keeping
		# a UI active and processing messages during a long-running
		# operation.

		after idle [list after 0 [list \
			delete_helper \
			$path_list \
			$path_index \
			$deletion_errors \
			$deletion_error_path \
			$batch_size \
			]]
	} else {
		# Finish the status bar operation.
		$::main_status stop

		# Report error, if any, based on how many deletions failed.
		set deletion_error_cnt [llength $deletion_errors]

		if {$deletion_error_cnt == 1} {
			error_popup [mc \
				"File %s could not be deleted: %s" \
				$deletion_error_path \
				[lindex $deletion_errors 0] \
				]
		} elseif {$deletion_error_cnt == $path_cnt} {
			error_popup [mc \
				"None of the selected files could be deleted." \
				]
		} elseif {$deletion_error_cnt > 1} {
			error_popup [mc \
				"%d of the selected files could not be deleted." \
				$deletion_error_cnt]
		}

		reshow_diff
		ui_ready
	}
}

# This function is from the TCL documentation:
#
#   https://wiki.tcl-lang.org/page/file+exists
#
# [file exists] returns false if the path does exist but is a symlink to a path
# that doesn't exist. This proc returns true if the path exists, regardless of
# whether it is a symlink and whether it is broken.
proc lexists name {
	expr {![catch {file lstat $name finfo}]}
}

# Remove as many empty directories as we can starting at the specified path.
# If we encounter a directory that is not empty, or if a directory deletion
# fails, then we stop the operation and return to the caller. Even if this
# procedure fails to delete any directories at all, it does not report failure.
proc remove_empty_directories {directory_path} {
	set parent_path [file dirname $directory_path]

	while {$parent_path != $directory_path} {
		set contents [glob -nocomplain -dir $directory_path *]

		if {[llength $contents] > 0} { break }
		if {[catch {file delete -- $directory_path}]} { break }

		set directory_path $parent_path
		set parent_path [file dirname $directory_path]
	}
}

proc do_revert_selection {} {
	global current_diff_path selected_paths

	if {[array size selected_paths] > 0} {
		revert_helper \
			[mc "Reverting selected files"] \
			[array names selected_paths]
	} elseif {$current_diff_path ne {}} {
		revert_helper \
			[mc "Reverting %s" [short_path $current_diff_path]] \
			[list $current_diff_path]
	}
}

proc do_select_commit_type {} {
	global commit_type commit_type_is_amend

	if {$commit_type_is_amend == 0
		&& [string match amend* $commit_type]} {
		create_new_commit
	} elseif {$commit_type_is_amend == 1
		&& ![string match amend* $commit_type]} {
		load_last_commit

		# The amend request was rejected...
		#
		if {![string match amend* $commit_type]} {
			set commit_type_is_amend 0
		}
	}
}
