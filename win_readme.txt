I've done a very quick and dirty port of your Sophia to windows.  By dirty I mean I just added lots of #ifdef _MSC_VER sections and didn't try to understand the code in depth.  I just swapped out or implemented the functions that didn't exist in windows.  I ran into a few snags along the way but the port seems mostly functional.  The snags I ran into are:


1) the lack of condition variables - I used pieces of code I found in an article by Douglas C. Schmidt and Irfan Pyarali
	The full article is at: http://www.cs.wustl.edu/~schmidt/win32-cv-1.html

2) typically, you can't delete/unlink a file while it's open if you used the CRT open function - I needed to change the way the file is opened to make this possible

3) you can't delete/unlink a file that is currently mapped - I found no way around this one and I had to reorder the code in Sophia.  I don't know if doing so is causing problems.

4) Windows doesn't offer an 8-bit atomic exchange function until win8, so I had to roll my own in assembly.  I'm 80% sure it's not total junk. ;)

5) sp_lockfile doesn't have a direct equivalent in windows.  I mean, you can lock the file or a range in it to limit multiple processes, but you can't lock it to prevent multiple threads and least of all, multiple handles.  A way to do it may be to introduce some lock map keyed by the file path (lower-cased path due to case-insensitivity).  For my needs this won't be required.  I'm single threading in a single process.  

6) spunlikely/splikely - I can't find a way to implement neither of these using MSC intrinsics or anything else on windows - so I've made them just just evaluate the expression.  I've run into what I can only guess to be a bug in the latest MSC in that if I don't also include the body of the "if" in braces, then I get parse errors.  I've been out of C/C++ as a day-in/day-out language long enough it could just be something I've done wrong.

7) Implemented the other pthreads needed code using as direct of replacements as I could

8) For the lack of fork, I haven't bothered re-implement the multi-process tests. For this and the lockfile reasons, only the first test is valid in concurrent.c.
