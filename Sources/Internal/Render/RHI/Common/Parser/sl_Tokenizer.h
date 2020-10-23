#pragma once

namespace sl
{
/** In addition to the values in this enum, all of the ASCII characters are
valid tokens. */
enum HLSLToken
{
    // Built-in types.
    HLSLToken_Float = 256,
    HLSLToken_Float2,
    HLSLToken_Float3,
    HLSLToken_Float4,
    HLSLToken_Float3x3,
    HLSLToken_Float4x4,
    HLSLToken_Half,
    HLSLToken_Half2,
    HLSLToken_Half3,
    HLSLToken_Half4,
    HLSLToken_Half3x3,
    HLSLToken_Half4x4,
    HLSLToken_Bool,
    HLSLToken_Int,
    HLSLToken_Int2,
    HLSLToken_Int3,
    HLSLToken_Int4,
    HLSLToken_Uint,
    HLSLToken_Uint2,
    HLSLToken_Uint3,
    HLSLToken_Uint4,
    HLSLToken_Texture,
    HLSLToken_Sampler,
    HLSLToken_Sampler2D,
    HLSLToken_Sampler3D,
    HLSLToken_SamplerCube,
    HLSLToken_Sampler2DShadow,
    HLSLToken_Sampler2DMS,

    // Reserved words.
    HLSLToken_If,
    HLSLToken_Else,
    HLSLToken_For,
    HLSLToken_While,
    HLSLToken_Break,
    HLSLToken_True,
    HLSLToken_False,
    HLSLToken_Void,
    HLSLToken_Struct,
    HLSLToken_VertexIn,
    HLSLToken_VertexOut,
    HLSLToken_FragmentIn,
    HLSLToken_FragmentOut,
    HLSLToken_CBuffer,
    HLSLToken_TBuffer,
    HLSLToken_Register,
    HLSLToken_Return,
    HLSLToken_Continue,
    HLSLToken_Discard,
    HLSLToken_Const,
    HLSLToken_Static,
    HLSLToken_Inline,

    // Input modifiers.
    HLSLToken_Uniform,
    HLSLToken_Property,
    HLSLToken_In,
    HLSLToken_Out,
    HLSLToken_InOut,

    // Effect keywords.
    HLSLToken_SamplerState,
    HLSLToken_Technique,
    HLSLToken_Pass,
    HLSLToken_Blending,
    HLSLToken_Blending0,
    HLSLToken_Blending1,
    HLSLToken_Blending2,
    HLSLToken_Blending3,
    HLSLToken_Blending4,
    HLSLToken_Blending5,
    HLSLToken_Blending6,
    HLSLToken_Blending7,
    HLSLToken_ColorMask,
    HLSLToken_ColorMask0,
    HLSLToken_ColorMask1,
    HLSLToken_ColorMask2,
    HLSLToken_ColorMask3,
    HLSLToken_ColorMask4,
    HLSLToken_ColorMask5,
    HLSLToken_ColorMask6,
    HLSLToken_ColorMask7,

    // Multi-character symbols.
    HLSLToken_LessEqual,
    HLSLToken_GreaterEqual,
    HLSLToken_EqualEqual,
    HLSLToken_NotEqual,
    HLSLToken_PlusPlus,
    HLSLToken_MinusMinus,
    HLSLToken_PlusEqual,
    HLSLToken_MinusEqual,
    HLSLToken_TimesEqual,
    HLSLToken_DivideEqual,
    HLSLToken_AndAnd, // &&
    HLSLToken_BarBar, // ||

    // Other token types.
    HLSLToken_FloatLiteral,
    HLSLToken_IntLiteral,
    HLSLToken_Identifier,

    HLSLToken_EndOfStream,
};

class HLSLTokenizer
{
public:
    /// Maximum string length of an identifier.
    static const int s_maxIdentifier = 255 + 1;

    /** The file name is only used for error reporting. */
    HLSLTokenizer(const char* fileName, const char* buffer, size_t length);

    /** Advances to the next token in the stream. */
    void Next();
    void ScanString();

    /** Returns the current token in the stream. */
    int GetToken() const;

    /** Returns the number of the current token. */
    float GetFloat() const;
    int GetInt() const;

    /** Returns the identifier for the current token. */
    const char* GetIdentifier() const;

    /** Returns the line number where the current token began. */
    int GetLineNumber() const;

    /** Returns the file name where the current token began. */
    const char* GetFileName() const;

    /** Gets a human readable text description of the current token. */
    void GetTokenName(char buffer[s_maxIdentifier]) const;

    /** Reports an error using printf style formatting. The current line number
    is included. Only the first error reported will be output. */
    void Error(const char* format, ...);

    /** Gets a human readable text description of the specified token. */
    static void GetTokenName(int token, char buffer[s_maxIdentifier]);

private:
    bool SkipWhitespace();
    bool SkipComment();
    bool ScanNumber();
    bool ScanLineDirective();

private:
    const char* m_fileName;
    const char* m_buffer;
    const char* m_bufferEnd;
    int m_lineNumber;
    bool m_error;

    int m_token;
    float m_fValue;
    int m_iValue;
    char m_identifier[s_maxIdentifier];
    char m_lineDirectiveFileName[s_maxIdentifier];
    int m_tokenLineNumber;
};

} // namespace sl
