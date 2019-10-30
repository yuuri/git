# SimpleChord class:
#   Represents a procedure that conceptually has multiple entrypoints that must
#   all be called before the procedure executes. Each entrypoint is called a
#   "note". The chord is only "completed" when all the notes are "activated".
#
#   Constructor:
#     set chord [SimpleChord new {body}]
#       Creates a new chord object with the specified body script. The body
#       script is evaluated at most once, when a note is activated and the
#       chord has no other non-activated notes.
#
#   Methods:
#     $chord eval {script}
#       Runs the specified script in the same context (namespace) in which the
#       chord body will be evaluated. This can be used to set variable values
#       for the chord body to use.
#
#     set note [$chord add_note]
#       Adds a new note to the chord, an instance of ChordNote. Raises an
#       error if the chord is already completed, otherwise the chord is updated
#       so that the new note must also be activated before the body is
#       evaluated.
#
#     $chord notify_note_activation
#       For internal use only.
#
# ChordNote class:
#   Represents a note within a chord, providing a way to activate it. When the
#   final note of the chord is activated (this can be any note in the chord,
#   with all other notes already previously activated in any order), the chord's
#   body is evaluated.
#
#   Constructor:
#     Instances of ChordNote are created internally by calling add_note on
#     SimpleChord objects.
#
#   Methods:
#     [$note is_activated]
#       Returns true if this note has already been activated.
#
#     $note
#       Activates the note, if it has not already been activated, and completes
#       the chord if there are no other notes awaiting activation. Subsequent
#       calls will have no further effect.
#
# Example:
#
#   # Turn off the UI while running a couple of async operations.
#   lock_ui
#
#   set chord [SimpleChord new {
#     unlock_ui
#     # Note: $notice here is not referenced in the calling scope
#     if {$notice} { info_popup $notice }
#   }
#
#   # Configure a note to keep the chord from completing until
#   # all operations have been initiated.
#   set common_note [$chord add_note]
#
#   # Pass notes as 'after' callbacks to other operations
#   async_operation $args [$chord add_note]
#   other_async_operation $args [$chord add_note]
#
#   # Communicate with the chord body
#   if {$condition} {
#     # This sets $notice in the same context that the chord body runs in.
#     $chord eval { set notice "Something interesting" }
#   }
#
#   # Activate the common note, making the chord eligible to complete
#   $common_note
#
# At this point, the chord will complete at some unknown point in the future.
# The common note might have been the first note activated, or the async
# operations might have completed synchronously and the common note is the
# last one, completing the chord before this code finishes, or anything in
# between. The purpose of the chord is to not have to worry about the order.

oo::class create SimpleChord {
	variable Notes
	variable Body
	variable IsCompleted

	constructor {body} {
		set Notes [list]
		set Body $body
		set IsCompleted 0
	}

	method eval {script} {
		namespace eval [self] $script
	}

	method add_note {} {
		if {$IsCompleted} { error "Cannot add a note to a completed chord" }

		set note [ChordNote new [self]]

		lappend Notes $note

		return $note
	}

	method notify_note_activation {} {
		if {!$IsCompleted} {
			foreach note $Notes {
				if {![$note is_activated]} { return }
			}

			set IsCompleted 1

			namespace eval [self] $Body
			namespace delete [self]
		}
	}
}

oo::class create ChordNote {
	variable Chord IsActivated

	constructor {chord} {
		set Chord $chord
		set IsActivated 0
	}

	method is_activated {} {
		return $IsActivated
	}

	method unknown {} {
		if {!$IsActivated} {
			set IsActivated 1
			$Chord notify_note_activation
		}
	}
}
