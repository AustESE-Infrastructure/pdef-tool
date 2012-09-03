mmpupload 1 22-8-2012 

NAME:
mmpupload - uploads a list of files and form fields

SYNOPSIS: 
mmpupload <folder>

DESCRIPTION:
Uploads a list of files and form values pairs to a server using the mime 
multipart format. It thus does what your browser does in uploading a set 
of files and field values during a HTML form submission, but on the 
commandline so it can be easily automated. 

mmpupload is much easier to use than curl, and is insensitive to spaces, 
braces and brackets in file paths.

mmpuload scans the source folder for literal paths, docids and config files.

LITERAL PATHS

A folder name beginning with '@' designates a literal path. The 
foldername minus the '@' designates the database to which the contained 
files will be uploaded. The remaining folders and files nested within a 
literal path folder form the docids. e.g. a file in a folder structure:

capuana/@config/stripper/play/italian/capuana.json

will upload the file "capuana.json" to the database "config" with the docid "stripper/play/italian/tei.json".

CONFIG FILES

In any part of the directory structure a JSON file ending in ".conf" can be placed. As the directory structure is navigated the config files are merged via a stack, that is config files in sibling branches are not merged, but child branches are merged with their parents. In this way the properties desired for various sections of the hierarchy can be specified as desired. 

Currently recognised keys are:
base_url:
which specifies the bare url to upload to (but not the resource path).

corform: 
specifies a corform file (a CSS file wrapped in JSON) as the default format for files in this and child directories.

stripper:
the docid of a stripper config file to direct stripping of markup from 
files in this an child directories.

splitter:
the splitter config to use for this and all child directories.

filter:
designates the name of a Java filter program to be used for filtering 
text files.

versions:
specifies an array of key-value pairs mapping short names (which must 
match the names and relative paths of the files minus their suffixes) 
and their long version names. For example, a file with a relative path 
poem2/A88.xml has a short name of poem2/A88.

DOCIDs: 
A folder name beginning with "+" forms the start of a docid for 
enclosed folders until another folder starting with "%" is encountered. 
e.g. the folder path:

+italian/capuana/aristofanunculos/%Frammento 1

designates a docid of "italian/capuana/aristofanunculos/Frammento 1". 
This docid is then used for all the files contained in that folder, 
which are merged into a pair of MVDs, a cortex and a corcode, which are 
then stored at:

cortex/italian/capuana/aristofanunculos/Frammento 1

and

corcode/italian/capuana/aristofanunculos/Frammento 1.

Any further folders within the path 
"+italian/capuana/aristofanunculos/%Frammento 1" become groups within 
the generated MVDs.

MVDs: An MVD folder is designated by a docid path as described above. 
Three types of content are allowed, and are each turned into a pair of 
corcode/cortex files stored at the same docid in two different databases 
(cortex and corcode).

1. If the docid path contains mostly text files the text files are 
converted into cortex and corcode via a special Java filter program.

2. If the docid path contains mostly XML files then the splitter and 
stripper programs are invoked on each file, and the results accumulated 
into cortex and corcodes, which are then merged at stored at the docid.

3. If the docid path contains two sub-directorties called "cortex" and 
"corcode" then the contents of these directories will be merged into 
their respective cortex and corcode files.

EXAMPLE:
mmpupload _harpur
