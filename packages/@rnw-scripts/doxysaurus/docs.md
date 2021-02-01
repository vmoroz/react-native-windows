

Use of hyphen, n-dash, m-dash
https://www.punctuationmatters.com/hyphen-dash-n-dash-and-m-dash/

In Markdown:
- hypen
-- en-dash
--- em-dash

\n - new line 

Escape (from https://www.doxygen.nl/manual/commands.html)
\\
This command writes a backslash character (\) to the output. The backslash has to be escaped in some cases because doxygen uses it to detect commands.

\@
This command writes an at-sign (@) to the output. The at-sign has to be escaped in some cases because doxygen uses it to detect Javadoc commands.

\&
This command writes the & character to the output. This character has to be escaped because it has a special meaning in HTML.

\$
This command writes the $ character to the output. This character has to be escaped in some cases, because it is used to expand environment variables.

\#
This command writes the # character to the output. This character has to be escaped in some cases, because it is used to refer to documented entities.

\<
This command writes the < character to the output. This character has to be escaped because it has a special meaning in HTML.

\>
This command writes the > character to the output. This character has to be escaped because it has a special meaning in HTML.

\%
This command writes the % character to the output. This character has to be escaped in some cases, because it is used to prevent auto-linking to a word that is also a documented class or struct.

\"
This command writes the " character to the output. This character has to be escaped in some cases, because it is used in pairs to indicate an unformatted text fragment.

\.
This command writes a dot (.) to the output. This can be useful to prevent ending a brief description when JAVADOC_AUTOBRIEF is enabled or to prevent starting a numbered list when the dot follows a number at the start of a line.

\=
This command writes an equal sign (=) to the output. This character sequence has to be escaped in some cases, because it is used in Markdown header processing.

\::
This command writes a double colon (::) to the output. This character sequence has to be escaped in some cases, because it is used to reference to documented entities.

\|
This command writes a pipe symbol (|) to the output. This character has to be escaped in some cases, because it is used for Markdown tables.

\--
This command writes two dashes (--) to the output. This allows writing two consecutive dashes to the output instead of one n-dash character (–).

\---
This command writes three dashes (---) to the output. This allows writing three consecutive dashes to the output instead of one m-dash character (—).


//! **bold** __bold__  \b bold not-bold <b>several words</b>
//!
//! *emphasis* _emphasis_ \a emphasis \e emphasis \em emphasis <em>several words</em>

Line break
//! text1 <br/> text2 \n
//! text3

//! Spaces: &nbsp;text&ensp;text&emsp;text&thinsp;text

//!
//! # heading 1
//!
//! ## heading 2
//!
//! ### heading 3
//!
//! #### heading 4
//!
//! ##### heading 5
//!
//! ###### heading 6

//! no heading 7
//!
//! Dashes: hypen:- ndash: -- mdash: ---
//! 
//! Spaces: &nbsp;text&ensp;text&emsp;text&thinsp;text
//!
//! ---
//!
//! * item1
//! * item2
//! * item3


@param
@retval
@return

