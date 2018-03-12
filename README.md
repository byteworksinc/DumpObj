# DumpObj
A utility designed for use with the ORCA development environment for the Apple IIGS, DumpObj prints a more-or-less human readable version of Apple IIGS object files.

__Binary downloads for the latest DumpObj release are on the [releases page][releases].__

[releases]: https://github.com/byteworksinc/DumpObj/releases

If you would like to make changes to this utility and distribute them to others, feel free to submit them here. If the changes apply to compilation on and for an Apple IIGS, they will generally be approved for distribution on the master branch.

The general conditions that must be met before a change is released on master are:

1. The modified utility must compile under the currently released version of ORCA/C.

Contact support@byteworks.us if you need contributor access.

A complete distribution of the ORCA languages, including installers and documentation, is available from the Juiced GS store at https://juiced.gs/store/category/software/. It is distributed as part of the Opus ][ package.

## Line Endings and File Types

The text and source files in this repository originally used CR line endings, as usual for Apple II text files, but they have been converted to use LF line endings because that is the format expected by Git. If you wish to move them to a real or emulated Apple II and build them there, you will need to convert them back to CR line endings.

If you wish, you can configure Git to perform line ending conversions as files are checked in and out of the Git repository. With this configuration, the files in your local working copy will contain CR line endings suitable for use on an Apple II. To set this up, perform the following steps in your local copy of the Git repository (these should be done when your working copy has no uncommitted changes):

1. Add the following lines at the end of the `.git/config` file:
```
[filter "crtext"]
	clean = LC_CTYPE=C tr \\\\r \\\\n
	smudge = LC_CTYPE=C tr \\\\n \\\\r
```

2. Add the following line to the `.git/info/attributes` file, creating it if necessary:
```
* filter=crtext
```

3. Run the following commands to convert the existing files in your working copy:
```
rm .git/index
git checkout HEAD -- .
```

Alternatively, you can keep the LF line endings in your working copy of the Git repository, but convert them when you copy the files to an Apple II. There are various tools to do this.  One option is `udl`, which is [available][udl] both as a IIGS shell utility and as C code that can be built and used on modern systems.

[udl]: http://ftp.gno.org/pub/apple2/gs.specific/gno/file.convert/udl.114.shk

In addition to converting the line endings, you will also have to set the files to the appropriate file types before building ORCA/C on a IIGS. The included `settypes` script (for use under the ORCA shell) does this for the sources to the ORCA/C compiler itself, although it does not currently cover the test cases and headers.
