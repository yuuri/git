# Color configuration support for git-gui.

namespace eval Color {
	# static colors
	variable lightRed		lightsalmon
	variable lightGreen		green
	variable lightGold		gold
	variable lightBlue		blue
	variable textOnLight	black
	variable textOnDark		white
	# theme colors
	variable interfaceBg	lightgray
	variable textBg			white
	variable textColor		black

	proc syncColorsWithTheme {} {
		set Color::interfaceBg	[ttk::style lookup Entry -background]
		set Color::textBg		[ttk::style lookup Treeview -background]
		set Color::textColor	[ttk::style lookup Treeview -foreground]

		tk_setPalette $Color::interfaceBg
	}
}
