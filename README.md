This is a fork of vislcg3 that adds some support for .NET P/Invoking. The following methods were added to src/libcg3.cpp & cg3.h:

```
// Runs a grammar on a UTF-8 string instead of stdin
char* cg3_run_grammar_on_text_str(cg3_applicator* applicator_, const char* is_);
```

```
// Frees the unmanaged string returned by cg3_run_grammar_on_text_str
void cg3_run_grammar_on_text_str_free(char *str);
```

#Installation

```
sudo apt-get install g++ libicu-dev subversion cmake libboost-dev build-essential
 ./cmake.sh
make -j3
./test/runall.pl
sudo make install
sudo ldconfig
```

See also http://beta.visl.sdu.dk/cg3/chunked/installation.html.
*DO NOT* add support for TMalloc. I.e. *DO NOT* run sudo apt-get install libgoogle-perftools-dev

#P/Invoking

```
class Example
{
    static IntPtr _grammar;

    static Example()
    {
        if ((_grammar = CG3.cg3_grammar_load(PATH_TO_GRAMMAR)) == IntPtr.Zero)
            throw new CG3GrammarException($"Failed to load CG3 grammar '{PATH_TO_GRAMMAR}'");
    }

    void Foo()
    {
        lock (synchronizationObject) // vislcg3 not thread-safe
        {
            IntPtr applicator = default(IntPtr);

            try
            {
                applicator = CG3.cg3_applicator_create(_grammar);
                CG3.cg3_applicator_setflags(applicator, (uint)cg3_flags.CG3F_NO_PASS_ORIGIN | (uint)cg3_flags.CG3F_SHOW_END_TAGS);
                output = CG3.cg3_run_grammar_on_text_str(applicator, inputString);
            }

            finally
            {
                if (applicator != default(IntPtr))
                    CG3.cg3_applicator_free(applicator);
            }
        }
    }
}
```
