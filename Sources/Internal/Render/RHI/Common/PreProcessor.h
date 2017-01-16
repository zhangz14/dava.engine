#pragma once

#include <vector>
#include "FileSystem/File.h"
#include "ExpressionEvaluator.h"

typedef std::vector<char> TextBuf;

class
PreProc
{
public:
    class FileCallback;

            PreProc();
            ~PreProc();

    bool    process_file( const char* file_name, TextBuf* output );
    bool    process_inplace( char* src_text, TextBuf* output );
    bool    process( const char* src_text, TextBuf* output );
    void    clear();

    void    dump() const;
    

public:

    class
    FileCallback
    {
    public:

        virtual             ~FileCallback()                             {}

        virtual bool        open( const char* /*file_name*/ )           { return false; }
        virtual void        close()                                     {}
        virtual unsigned    size() const                                { return 0; }
        virtual unsigned    read( unsigned /*max_sz*/, void* /*dst*/ )  { return 0; }
    };


private:        

    struct Line;
    struct Buffer;
    struct Var;

    void    _reset();
    char*   _alloc_buffer( unsigned sz );
    bool    _process_buffer( char* text, std::vector<Line>* line );
    bool    _process_include( const char* file_name, std::vector<Line>* line );
    bool    _process_define( const char* name, const char* val );
    void    _generate_output( TextBuf* output );


    struct
    Line
    {
        char*       text;
        unsigned    line_n;
                    
                    Line( char* t, unsigned n )
                      : text(t), line_n(n)
                    {}
    };

    struct
    Buffer
    {
        void*   mem;
    };

    struct
    Var
    {
        char    name[64];
        int     val;
    };
    
    std::vector<Buffer>     _buf;
    std::vector<Line>       _line;
    std::vector<Var>        _var;
    ExpressionEvaluator     _eval;
    
    FileCallback*           _file_cb;

    class
    DefFileCallback
      : public FileCallback
    {
    public:

        virtual bool        open( const char* file_name )
                            {
                                _in = DAVA::File::Create( file_name, DAVA::File::READ|DAVA::File::OPEN );
                                return (_in)  ? true  : false;
                            }
        virtual void        close()
                            {
                                DVASSERT(_in);
                                _in->Release();
                                _in = nullptr;
                            }
        virtual unsigned    size() const
                            {
                                return (_in)  ? unsigned(_in->GetSize())  : 0;
                            }
        virtual unsigned    read( unsigned max_sz, void* dst )  
                            { 
                                return (_in)  ? _in->Read( dst, max_sz )  : 0; 
                            }

    private:

        DAVA::File* _in;
    };


    static DefFileCallback  _DefFileCallback;
};
